#include "AtomBlockDecoder.h"
#include "../shared/WaveSampleTypes.h"
#include <iostream>
#include "../shared/Debug.h"
#include "../shared/Utility.h"


string AtomBlockDecoder::atomTapeBlockHdrFieldName(int offset)
{
	switch (offset) {
	case 0: return "flags";
	case 1: return "blockNoHigh";
	case 2: return "blockNoLow";
	case 3: return "dataLenM1";
	case 4: return "execAdrHigh";
	case 5: return "execAdrLow";
	case 6: return "loadAdrHigh";
	case 7: return "loadAdrLow";
	default: return "???";
	}

}


AtomBlockDecoder::AtomBlockDecoder(
	CycleDecoder& cycleDecoder, ArgParser &argParser, bool verbose) : BlockDecoder(cycleDecoder, argParser, verbose, ACORN_ATOM)
{
}


/*
 * 
 * Read a complete Acorn Atom tape block, starting with the detection of its lead tone and
 * ending with the reading of the stop bit of the CRC byte that ends the block.
 * 
 */
bool AtomBlockDecoder::readBlock(
	double leadToneDuration, FileBlock& readBlock,
	BlockType& block_type, int& blockNo, bool& leadToneDetected
)
{
	uint16_t CRC;
	nReadBytes = 0;
	mTapeFileName = "???";

	if (mVerbose)
		cout << "\n\nReading a new block...\n";

	// Invalidate output variables in case readBlock returns prematurely
	block_type = BlockType::Unknown;
	blockNo = -1;
	leadToneDetected = false;

	// Invalidate block data in case readBlock returns with only partilly filled out block
	readBlock.atomHdr.execAdrHigh = 0xff;
	readBlock.atomHdr.execAdrLow = 0xff;
	readBlock.atomHdr.lenHigh = 0xff;
	readBlock.atomHdr.lenLow = 0xff;
	readBlock.atomHdr.loadAdrHigh = 0xff;
	readBlock.atomHdr.loadAdrLow = 0xff;
	readBlock.atomHdr.name[0] = 0xff;

	// Wait for lead tone of a min duration but still 'consume' the complete tone
	double duration, dummy;
	if (!mCycleDecoder.waitForTone(leadToneDuration, duration, dummy, readBlock.leadToneCycles, mLastHalfCycleFrequency)) {
		// This is not necessarily an error - it could be because the end of the tape as been reached...
		return false;
	}
	leadToneDetected = true;
	if (mVerbose)
		cout << duration << "s lead tone detected at " << Utility::encodeTime(getTime()) << "\n";

	// Read block header's preamble (i.e., synhronisation bytes)
	if (!checkBytes(0x2a, 4)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read header preamble (0x2a bytes)%s\n", "");
		return false;
	}

	// Get phaseshift when transitioning from lead tone to start bit.
	// (As recorded by the Cycle Decoder when reading the preamble.)
	readBlock.phaseShift = mCycleDecoder.getPhaseShift();

	// Initialize CRC with sum of preamble
	CRC = 0x2a * 4;

	// Read block header's name (1 to 13 characters + 0xd terminator) field
	int name_len;
	if (!getFileName(readBlock.atomHdr.name, CRC, name_len)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read header block name%s\n", "");
		return false;
	}

	// Remember tape file name (for output of debug/error messages)
	mTapeFileName = readBlock.atomHdr.name;

	// Read Atom tape header bytes
	Byte* hdr = (Byte*)&mAtomTapeBlockHdr;
	int collected_stop_bit_cycles;
	for (int i = 0; i < sizeof(mAtomTapeBlockHdr); i++) {
		if (!getByte(hdr, collected_stop_bit_cycles)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to read header %s\n", atomTapeBlockHdrFieldName(i).c_str());
			return false;
		}
		Utility::updateCRC(mTargetMachine, CRC, *hdr++);
	}

	int load_address, exec_adr, len;
	if (!Utility::decodeAtomTapeHdr(mAtomTapeBlockHdr, readBlock, load_address, exec_adr, len, blockNo, block_type))
		return false;


	if (mVerbose)
		cout << "Header " << mTapeFileName << " " << hex << mAtomTapeBlockHdr.loadAdrHigh * 256 + mAtomTapeBlockHdr.loadAdrLow << " " << mAtomTapeBlockHdr.dataLenM1 + 1 << " " << blockNo << " read at " << Utility::encodeTime(getTime()) << dec << "\n";


	// Consume micro lead tone between header and data block (to record it's duration only)
	int micro_tone_half_cycles;
	if (!mCycleDecoder.stopOnHalfCycles(Frequency::F2, 1, dummy, mLastHalfCycleFrequency)) { // Find the  first F2 1/2 cycle
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to detect a start of data tone for file '%s'\n", readBlock.atomHdr.name);
		return false;
	}
	if (!mCycleDecoder.consumeHalfCycles(Frequency::F2, micro_tone_half_cycles, mLastHalfCycleFrequency)) { // Consume the remaining 1/2 cycles
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to detect a start of data tone for file '%s'\n", readBlock.atomHdr.name);
		return false;
	}
	micro_tone_half_cycles++;
	double micro_tone_duration = (double)micro_tone_half_cycles / (F2_FREQ * 2);
	mArgParser.tapeTiming.minBlockTiming.microLeadToneDuration = micro_tone_duration;

	if (mVerbose)
		cout << micro_tone_duration << "s micro tone detected at " << Utility::encodeTime(getTime()) << "\n";

	// Get data bytes
	if (len > 0) {
		if (!getBytes(readBlock.data, len, CRC)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to read block data for file '%s'!\n", readBlock.atomHdr.name);
			return false;
		}
	}

	if (mVerbose) {
		cout << "Data bytes: ";
		for (int i = 0; i < len; i++)
			cout << _BYTE(readBlock.data[i]) << " ";
		cout << "\n";

		cout << len << " bytes of Block data read at " << Utility::encodeTime(getTime()) << "\n";
	}

	// Get CRC
	Byte received_CRC;
	mReadingCRC = true;
	if (!getByte(&received_CRC, collected_stop_bit_cycles)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read CRC for file '%s'\n", readBlock.atomHdr.name);
		return false;
	}
	mReadingCRC = false;

	if (mVerbose)
		cout << "CRC read at " << Utility::encodeTime(getTime()) << "\n";

	// Check CRC
	if (received_CRC != (CRC  & 0xff)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Calculated CRC 0x%x differs from received CRC 0x%x for file '%s'\n", CRC, received_CRC, readBlock.atomHdr.name);
		return false;
	}

	if (mVerbose) {
		cout << len << " bytes data block + CRC read at " << Utility::encodeTime(getTime()) << "\n";
	}


	// Detect gap to next block by waiting for 100 cycles of the next block's lead tone
	// After detection, there will be a roll back to the end of the block
	// so that the next block detection will not miss the lead tone.
	checkpoint();
	Frequency dummy_freq;
	if (!mCycleDecoder.stopOnHalfCycles(Frequency::F2, 100, readBlock.blockGap, dummy_freq)) {
		readBlock.blockGap = 2.0;
	}
	rollback();

	if (mVerbose)
		cout << readBlock.blockGap << " s gap after block, starting at " << Utility::encodeTime(getTime()) << "\n";

	return true;
}


bool AtomBlockDecoder::getFileName(char name[ATM_HDR_NAM_SZ], uint16_t &CRC, int &len) {
	int i = 0;
	Byte byte;
	bool failed = false;
	len = 0;
	do {
		if (!getByte(&byte)) {
			return false;
		}
		else if (byte != 0xd)
			name[i] = (char)byte;
		else
			len = i;
		i++;
		Utility::updateCRC(mTargetMachine, CRC, byte);
	} while (!failed && byte != 0xd && i <= ATOM_TAPE_NAME_LEN);

	// Add zero-padding
	for (int j = len; j < ATM_HDR_NAM_SZ; j++)
		name[j] = '\0';

	return true;

}



