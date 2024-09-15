#include "BlockDecoder.h"
#include "../shared/WaveSampleTypes.h"
#include <iostream>
#include "../shared/Debug.h"
#include "../shared/Utility.h"


BlockDecoder::BlockDecoder(
	TapeReader& tapeReader, ArgParser& argParser) :
	mReader(tapeReader), mArgParser(argParser), mVerbose(argParser.verbose), mTargetMachine(argParser.targetMachine), mTracing(argParser.tracing)
{
	nReadBytes = 0; // Not needed but made to make compiler happy
}



bool BlockDecoder::checkByte(Byte refVal, Byte& readVal) {
	if (!mReader.readByte(readVal) || readVal != refVal) {
		return false;
	}
	return true;
}



bool BlockDecoder::checkBytes(Bytes &bytes, Byte refVal, int n) {
	bool failed = false;
	int start_cycle = 0;
	for (int i = start_cycle; i < n && !failed; i++) {
		Byte val = 0;
		if (!checkByte(refVal, val)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Byte #%d (%.2x) out of %d bytes differs from reference value 0x%.2x or couldn't be read!\n", i, val, n, refVal);
			failed = true;
		}
		bytes.push_back(val);
	}
	return !failed;
}

bool BlockDecoder::getWord(Word* word)
{
	Bytes bytes;
	if (!mReader.readBytes(bytes, 2)) {
		return false;
	}
	*word = bytes[0] * 256 + bytes[1];
	return true;
}

/*
 *
 * Read a complete tape block, starting with the detection of its lead tone and
 * ending with the detection of the end of the block.
 *
 * For BBC Machines (except the Electron), the timing and structure of a Tape File is
 * <0.625s lead tone prelude > <dummy byte> <0.625s lead tone postlude>] <header with CRC>
 *  {<data with CRC> <0.25s lead tone> <data with CRC } <0.83s trailer tone> <3.3s gap>
 *
 * For the electron the timing is as above except that there is no dummy byte inserted.
 * 
 * For Atom, the the timing and the structure of a Tape file is
 * <4s lead tone> <header> <0.5s micro tone> <data> <2s gap> { <2s lead tone> <header> <0.5s micro tone> <data> <2s gap> }
 *
 * The blockTiming parameter defines what timing shall be used when reding the block. Normally shorter
 * values than above are used to have some margin.
 * 
 */
bool BlockDecoder::readBlock(BlockTiming blockTiming, bool firstBlock, FileBlock& readBlock, bool& leadToneDetected)
{
	nReadBytes = 0;
	leadToneDetected = false;

	if (!(mTargetMachine <= BBC_MASTER || mTargetMachine == ACORN_ATOM))
		return false;

	// Invalidate block in case readBlock returns prematurely
	readBlock.init();

	// Mark the block's target machine
	readBlock.targetMachine = mTargetMachine;

	// Wait for carrier
	double waiting_time;
	if (firstBlock) {
		int lead_tone_cycles = (int) round(blockTiming.firstBlockLeadToneDuration * mReader.carrierFreq());
		if (mTargetMachine <= BBC_MASTER) {
			if (!mReader.waitForCarrierWithDummyByte(
				blockTiming.firstBlockPreludeLeadToneCycles, lead_tone_cycles, 0xaa, waiting_time,
				readBlock.preludeToneCycles, readBlock.leadToneCycles
			))
				return false;
		}
		else {
			if (!mReader.waitForCarrier(lead_tone_cycles, waiting_time, readBlock.leadToneCycles))
				return false;
		}
	}
	else {
		int lead_tone_cycles = (int) round(blockTiming.otherBlockLeadToneDuration * mReader.carrierFreq());
		if (!mReader.waitForCarrier(lead_tone_cycles, waiting_time, readBlock.leadToneCycles))
			return false;
	}
	leadToneDetected = true;

	// Get phaseshift when transitioning from lead tone to start bit.
	readBlock.phaseShift = mReader.getPhaseShift();

	Word crc = 0x0;

	// Read block header's preamble (i.e., synchronisation bytes)
	int nPremable = 1;
	Bytes preamble_bytes;
	if (mTargetMachine == ACORN_ATOM)
		nPremable = 4;
	if (!checkBytes(preamble_bytes, 0x2a, nPremable)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read header preamble%s\n", "");
		return false;
	}
	if (mTargetMachine == ACORN_ATOM) // For Atom the premable is included in the CRC
		updateCRC(readBlock, crc, preamble_bytes);


	// Get phaseshift when transitioning from lead tone to start bit.
	// (As recorded by the Cycle Decoder when reading the preamble.)
	readBlock.phaseShift = mReader.getPhaseShift();

	// Read block name
	Bytes name_bytes;
	if (!getBlockName(name_bytes)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read header block name%s\n", "");
		return false;
	}
	updateCRC(readBlock, crc, name_bytes);

	// Read rest of header (excluding any CRC)
	Bytes hdr_bytes;
	int n_read_bytes; // see how many bytes was successfully read
	if (!mReader.readBytes(hdr_bytes, readBlock.tapeHdrSz(), n_read_bytes)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read header field %s\n", readBlock.tapeField(n_read_bytes).c_str());
		nReadBytes += n_read_bytes;
		return false;
	}
	nReadBytes += n_read_bytes;
	updateCRC(readBlock, crc, hdr_bytes);

	// Decode header
	if (!readBlock.decodeTapeHdr(name_bytes, hdr_bytes)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to decode header for file '%s'\n", readBlock.blockName().c_str());
		return false;
	}

	if (mVerbose)
		readBlock.logHdr();

	if (readBlock.targetMachine <= BBC_MASTER) { // For all but Atom, the header has a separate CRC

		// Get header CRC
		Word hdr_CRC;
		if (!getWord(&hdr_CRC)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to read header CRC for file '%s'\n", readBlock.blockName().c_str());
			return false;
		}
		nReadBytes += 2;

		// Check against calculated header CRC
		if (hdr_CRC != crc) {
			if (mTracing) {
				DEBUG_PRINT(getTime(), ERR, "Calculated header CRC 0x%x differs from received CRC 0x%x for file '%s'\n", crc, hdr_CRC,
					readBlock.blockName().c_str());
			}
			return false;
		}

		if (mVerbose)
			cout << "A correct header CRC 0x" << hex << hdr_CRC << " read at " << Utility::encodeTime(getTime()) << "\n";

		crc = 0x0; // reset CRC for use as data CRC

	}

	// For Atom, detect micro tone before reading data bytes
	if (readBlock.targetMachine == ACORN_ATOM) {
		double waiting_time;
		if (!mReader.waitForCarrier(100, waiting_time, readBlock.microToneCycles)) {
		//if (!mReader.consumeCarrier(0.1, readBlock.microToneCycles)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to detect micro tone for file '%s'!\n", readBlock.blockName().c_str());
		}

		double micro_tone_duration = readBlock.microToneCycles / mReader.carrierFreq();
		if (mVerbose)
			cout << micro_tone_duration << "s (" << dec << readBlock.microToneCycles << " cycles) micro tone detected at " << Utility::encodeTime(getTime()) << "\n";
	}


	// Get data bytes
	int block_len = readBlock.dataSz();
	if (!mReader.readBytes(readBlock.data, block_len, n_read_bytes)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read block data for file '%s'!\n", readBlock.blockName().c_str());
		nReadBytes += n_read_bytes;
		return false;
	}
	updateCRC(readBlock, crc, readBlock.data);
	nReadBytes += n_read_bytes;

	if (mVerbose)
		cout << dec << n_read_bytes << " bytes of expected " << block_len << " of data bytes read at " << Utility::encodeTime(getTime()) << "\n";

	if (DEBUG_LEVEL == DBG && mTracing)
		Utility::logData(readBlock.loadAdr(), &readBlock.data[0], block_len);

	// Get data CRC
	Word data_CRC;
	if (readBlock.targetMachine <= BBC_MASTER) {
		if (!getWord(&data_CRC)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to read block CRC for file '%s'\n", readBlock.blockName().c_str());
			return false;
		}
		nReadBytes += 2;
	}
	else {
		Byte c;
		if (!mReader.readByte(c)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to read block CRC for file '%s'\n", readBlock.blockName().c_str());
			return false;
		}
		nReadBytes += 1;
		data_CRC = c;
	}

	// Check against calculated CRC
	if (data_CRC != crc) {
		if (true||mTracing)
			DEBUG_PRINT(getTime(), ERR, "Calculated (data) CRC 0x%x differs from received  CRC 0x%x for file '%s'\n", crc, data_CRC,
				readBlock.blockName().c_str());
		return false;
	}


	if (mVerbose)
		cout << "A correct (data) CRC 0x" << hex << data_CRC << " read at " << Utility::encodeTime(getTime()) << "\n";
		
	// For BBC machines, skip trailer tone that only exists for the last block
	if (readBlock.targetMachine <= BBC_MASTER && readBlock.lastBlock()) {
		double waiting_time;
		int trailer_tone_cycles = (int) round(blockTiming.trailerToneDuration * mReader.carrierFreq());
		if (!mReader.waitForCarrier(trailer_tone_cycles, waiting_time, readBlock.trailerToneCycles)) {
			// This is not necessarily an error - it could be because the end of the tape as been reached...
			return false;
		}

		if (mVerbose) {
			double duration = (double) readBlock.trailerToneCycles / mReader.carrierFreq();
			cout << duration << "s (" << blockTiming.trailerToneDuration << "s) trailer tone detected at " << Utility::encodeTime(getTime()) << "\n";
		}
	}

	// For all Atom blocks and for last BBC machine block, record gap after block
	if (readBlock.targetMachine == ACORN_ATOM || (readBlock.targetMachine <= BBC_MASTER && readBlock.lastBlock())) {

		// Detect gap to next file by waiting for 100 cycles of the next files's first block's lead tone
		// After detection, there will be a roll back to the end of the block
		// so that the next block detection will not miss the lead tone.
		checkpoint();
		FileBlock saved_block = readBlock;
		int carrier_cycles;
		double waiting_time;
		double next_block_lead_tone_duration;
		if (readBlock.lastBlock())
			next_block_lead_tone_duration = blockTiming.firstBlockLeadToneDuration;
		else
			next_block_lead_tone_duration = blockTiming.otherBlockLeadToneDuration;
		int  next_block_lead_tone_cycles = (int) round(next_block_lead_tone_duration * mReader.carrierFreq());
		if (readBlock.targetMachine == ACORN_ATOM) {
			if (!mReader.waitForCarrier(next_block_lead_tone_cycles, waiting_time, carrier_cycles))
				// If no gap detected (probably because the tape has ended), use a default gap of 2s
				waiting_time = 2.0;
		}
		else { // BBC Machine
			if (readBlock.lastBlock()) {
				if (!mReader.waitForCarrierWithDummyByte(
					blockTiming.firstBlockPreludeLeadToneCycles, next_block_lead_tone_cycles, 0xaa, waiting_time,
					readBlock.preludeToneCycles, readBlock.leadToneCycles
				))
					// If no gap detected (probably because the tape has ended), use a default gap of 2s
					waiting_time = 2.0;
			}
			else
				waiting_time = 0.0; // No gap between BBC Machine blocks
		}
		rollback();
		readBlock = saved_block;
		readBlock.blockGap = waiting_time;

		if (mVerbose)
			cout << readBlock.blockGap << " s gap after block, starting at " << Utility::encodeTime(getTime()) << "\n";
	}

	return true;
}

bool BlockDecoder::getBlockName(Bytes &name)
{

	Byte b;
	Byte terminator = 0x0;
	int max_len = BBM_TAPE_NAME_LEN;
	int max_ATM_len = BTM_HDR_NAM_SZ;
	if (mTargetMachine == ACORN_ATOM) {
		terminator = 0xd;
		max_len = ATOM_TAPE_NAME_LEN;
		max_ATM_len = ATM_HDR_NAM_SZ;
	}

	int i = 0;
	do {
		if (!mReader.readByte(b)) {
			return false;
		}
		name.push_back(b);
		i++;
		nReadBytes++;
	} while (b != terminator && i <= max_len);


	return true;
}


bool BlockDecoder::updateCRC(FileBlock block, Word& crc, Bytes data)
{
	if (data.size() == 0)
		return false;

	for(int i = 0; i < data.size(); block.updateCRC(crc,data[i++]));

	return true;
}

// Save the current file position
bool BlockDecoder::checkpoint()
{
	return mReader.checkpoint();
}

// Roll back to a previously saved file position
bool BlockDecoder::rollback()
{
	return mReader.rollback();
}