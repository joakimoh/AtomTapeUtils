#include "BlockDecoder.h"
#include "../shared/WaveSampleTypes.h"
#include <iostream>
#include "../shared/Debug.h"
#include "../shared/Utility.h"

string atomTapeBlockHdrFieldName(int offset)
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

// Save the current file position
bool BlockDecoder::checkpoint()
{
	if (!mCycleDecoder.checkpoint()) {
		return false;
	}

	return true;
}

// Roll back to a previously saved file position
bool BlockDecoder::rollback()
{
	if (!mCycleDecoder.rollback()) {
		return false;
	}
	return true;
}

BlockDecoder::BlockDecoder(
	CycleDecoder& cycleDecoder, ArgParser &argParser, bool verbose) : mCycleDecoder(cycleDecoder), mArgParser(argParser), mVerbose(verbose)
{

	mTracing = argParser.tracing;

	if (mArgParser.tapeTiming.baudRate == 300) {
		mStartBitCycles = 4; // Start bit length in cycles of F1 frequency carrier
		mLowDataBitCycles = 4; // Data bit length in cycles of F1 frequency carrier
		mHighDataBitCycles = 8; // Data bit length in cycles of F2 frequency carrier
		mStopBitCycles = 9; // Stop bit length in cycles of F2 frequency carrier
	}
	else if (mArgParser.tapeTiming.baudRate == 1200) {
		mStartBitCycles = 1; // Start bit length in cycles of F1 frequency carrier
		mLowDataBitCycles = 1; // Data bit length in cycles of F1 frequency carrier
		mHighDataBitCycles = 2; // Data bit length in cycles of F2 frequency carrier
		mStopBitCycles = 3; // Stop bit length in cycles of F2 frequency carrier
	}
	else {
		throw invalid_argument("Unsupported baud rate " + mArgParser.tapeTiming.baudRate);
	}

	if (argParser.mErrorCorrection)
		mErrorCorrection = true;
}


/*
 * 
 * Read a complete Acorn Atom tape block, starting with the detection of its lead tone and
 * ending with the reading of the stop bit of the CRC byte that ends the block.
 * 
 */
bool BlockDecoder::readBlock(
	double leadToneDuration, ATMBlock &readBlock,
	BlockType &block_type, int& blockNo, bool &leadToneDetected
)
{
	Byte CRC;
	nReadBytes = 0;

	if (mVerbose)
		cout << "\n\nReading a new block...\n";

	// Invalidate output variables in case readBlock returns prematurely
	block_type = BlockType::Unknown;
	blockNo = -1;
	leadToneDetected = false;

	// Invalidate block data in case readBlock returns with only partilly filled out block
	readBlock.hdr.execAdrHigh = 0xff;
	readBlock.hdr.execAdrLow = 0xff;
	readBlock.hdr.lenHigh = 0xff;
	readBlock.hdr.lenLow = 0xff;
	readBlock.hdr.loadAdrHigh = 0xff;
	readBlock.hdr.loadAdrLow = 0xff;
	readBlock.hdr.name[0] = 0xff;

	// Wait for lead tone
	double duration, dummy;
	if (!mCycleDecoder.waitForTone(leadToneDuration, duration, dummy, readBlock.leadToneCycles)) {
		// This is not necessarily an error - it could be because the end of the tape as been reached...
		return false;
	}
	leadToneDetected = true;
	if (mVerbose)
		cout << duration << " lead tone detected at " << encodeTime(getTime()) << "\n";

	// Read block header's preamble (i.e., synhronisation bytes)
	if (!checkBytes(0x2a, 4)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read header preamble (0x2a bytes)%s\n", "");
		return false;
	}

	// Get phaseshift when transitioning from lead tone to start bit.
	// (As recorded by the Cycle Decoder when reading the preamble.)
	readBlock.phaseShift = mCycleDecoder.getPhase();

	// Initialize CRC with sum of preamble
	CRC = (Byte) (0x2a * 4);

	// Read block header's name (1 to 13 characters + 0xd terminator) field
	int name_len;
	if (!getFileName(readBlock.hdr.name, CRC, name_len)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read header block name%s\n", "");
		return false;
	}

	// Read Atom tape header bytes
	Byte * hdr = (Byte*)&mAtomTapeBlockHdr;
	int collected_stop_bit_cycles;
	for (int i = 0; i < sizeof(mAtomTapeBlockHdr); i++) {
		if (!getByte(hdr, collected_stop_bit_cycles)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to read header %s\n", atomTapeBlockHdrFieldName(i).c_str());
			return false;
		}
		CRC += *hdr++;
	}

	if (mVerbose)
		cout << "Header read at " << encodeTime(getTime()) << "\n";

	// Extract address information and update ATM block with it
	readBlock.hdr.execAdrHigh = mAtomTapeBlockHdr.execAdrHigh;
	readBlock.hdr.execAdrLow = mAtomTapeBlockHdr.execAdrLow;
	readBlock.hdr.loadAdrHigh = mAtomTapeBlockHdr.loadAdrHigh;
	readBlock.hdr.loadAdrLow = mAtomTapeBlockHdr.loadAdrLow;

	// Extract block no (returned to caller of readBlock)
	blockNo = mAtomTapeBlockHdr.blockNoHigh * 256 + mAtomTapeBlockHdr.blockNoLow;


	// Extract data length information and update ATM block with it
	int len;
	block_type = parseBlockFlag(mAtomTapeBlockHdr, len);
	readBlock.hdr.lenHigh = len / 256;
	readBlock.hdr.lenLow = len % 256;


	
	// Detect micro lead tone between header and data block
	if (collected_stop_bit_cycles == mStopBitCycles) {
		if (!mCycleDecoder.waitForTone(mArgParser.tapeTiming.minBlockTiming.microLeadToneDuration, duration, dummy, readBlock.microToneCycles)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to detect a start of data tone for file '%s'\n", readBlock.hdr.name);
		}
		if (mVerbose)
			cout << duration << " s micro tone detected at " << encodeTime(getTime()) << "\n";
	}
	else {
		DEBUG_PRINT(getTime(), ERR, "Header for file '%s' ended with a short stop bit (%d cycles) and no lead tone\n", readBlock.hdr.name, collected_stop_bit_cycles);
	}

	

	// Get data bytes
	if (len > 0) {
		if (!getBytes(readBlock.data, len, CRC)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to read data!%s\n", "");
			return false;
		}
	}

	// Get CRC
	uint8_t received_CRC = 0;
	if (!getByte(&received_CRC, collected_stop_bit_cycles)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read CRC for file '%s'\n", readBlock.hdr.name);
		return false;
	}

	// Check CRC
	if (received_CRC != CRC) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Calculated CRC 0x%x differs from received CRC 0x%x for file '%s'\n", CRC, received_CRC, readBlock.hdr.name);
		return false;
	}

	if (mVerbose)
		cout << len << " bytes data block + CRC read at " << encodeTime(getTime()) << "\n";


	// Detect gap to next block by waiting for the next block's lead tone
	// After detection, there will be a roll back to the end of the block
	// so that the next block detection will not miss the lead tone.
	checkpoint();
	int dummy_cycles;
	if (!mCycleDecoder.waitForTone(leadToneDuration, duration, readBlock.blockGap, dummy_cycles)) {
		// Must either be end of tape, start of a different file, or a corrupted tape
		// The block will therefore be assumed to be the last block of a file with a default gap of 2 s
		readBlock.blockGap = 2.0;
	}
	rollback();

	if (mVerbose)
		cout << readBlock.blockGap << " s gap after block, starting at " << encodeTime(getTime()) << "\n";

	return true;
}

bool BlockDecoder::checkByte(Byte refVal, Byte &readVal) {
	if (!getByte(&readVal) || readVal != refVal) {
		return false;
	}
	return true;
}



bool BlockDecoder::checkBytes(Byte refVal, int n) {
	bool failed = false;
	int start_cycle = 0;
	for (int i = start_cycle; i < n && !failed; i++) {
		Byte val = 0;
		if (!checkByte(refVal, val)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Byte #%d (%.2x) out of %d bytes differs from reference value 0x%.2x or couldn't be read!\n", i, val, n, refVal);
			failed = true;
		}
	}
	return !failed;
}

bool BlockDecoder::getFileName(char name[13], Byte &CRC, int &len) {
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
		CRC += byte;
	} while (!failed && byte != 0xd && i < 14);

	// Add zero-padding
	for (int j = len; j < 13; j++)
		name[j] = '\0';

	return true;

}


bool BlockDecoder::getBytes(Bytes &bytes, int n, Byte &CRC) {
	bool failed = false;
	for (int i = 0; i < n && !failed; i++) {
		Byte byte;
		if (!getByte(&byte)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to read byte #%d out of %d bytes\n", i, n);
			failed = true;
		}
		CRC += byte;
		bytes.push_back(byte);
	}
	return !failed;
}

//
// Detect a start bit by looking for exactly mStartBitCycles low tone cycles
//
bool BlockDecoder::getStartBit()
{

	CycleDecoder::CycleSample first_cycle_sample = mCycleDecoder.getCycle();

	// One cycle of the start bit has already been sampled when reading the stop bit or a lead tone - check that it was an F1 cycle
	if (mCycleDecoder.getCycle().freq != Frequency::F1) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "start bit cycle already sampled was not an F1 (but an %s)\n", _FREQUENCY(mCycleDecoder.getCycle().freq));
		return false;
	}

	CycleDecoder::CycleSample last_cycle_sample = mCycleDecoder.getCycle();

	// Collect the remaining cycles and check that they also where of type F1
	int n_collected_cycles;
	if (!mCycleDecoder.collectCycles(Frequency::F1, mStartBitCycles - 1, last_cycle_sample, n_collected_cycles)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed when collecting F1 cycles - last sampled cycle was %s\n", _FREQUENCY(last_cycle_sample.freq));
		return false;
	}

	return true;
}


double BlockDecoder::getTime() {
	return mCycleDecoder.getTime();
}

bool BlockDecoder::getDataBit(Bit& bit)
{

	CycleDecoder::CycleSample first_cycle_sample, last_cycle_sample;

	// Get one cycle of either F1 or F2 type to determine how many more cycles to read
	if (!mCycleDecoder.getNextCycle(first_cycle_sample) || !(first_cycle_sample.freq == Frequency::F1 || first_cycle_sample.freq == Frequency::F2))
	{
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "First cycle of data bit was an illegal %s cycle\n", _FREQUENCY(first_cycle_sample.freq));
		return false;
	}
	
	if (first_cycle_sample.freq == Frequency::F1) {
		int n_collected_cycles;
		if (!mCycleDecoder.collectCycles(Frequency::F1, mLowDataBitCycles - 1, last_cycle_sample, n_collected_cycles)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "No of F1 Cycles for databit was %d when expecting %d\n", n_collected_cycles + 1, mLowDataBitCycles);
			return false;
		}
		// A "0" bit
		bit = LowBit;
	}
	else if (first_cycle_sample.freq == Frequency::F2) {
		int n_collected_cycles;
		if (!mCycleDecoder.collectCycles(Frequency::F2, mHighDataBitCycles - 1, last_cycle_sample, n_collected_cycles)) {
			// If error correction is enabled, then accept 1 lost F2 cycle
			if (!mErrorCorrection || n_collected_cycles < mHighDataBitCycles - 1) {
				if (mTracing)
					DEBUG_PRINT(getTime(), ERR, "No of F2 Cycles for databit was %d when expecting %d\n", n_collected_cycles + 1, mHighDataBitCycles);
				return false;
			}
		}
		// An "1" bit
		bit = HighBit;
	}
	else {
		// Not a valid data bit
		return false;
	}

	return true;

}

//
// Detect a stop bit by looking for least one and maximum mStopBitCycles high tone (F2) cycles
// (Even if there should normally be mStopBitCycles cycles)
//
bool BlockDecoder::getStopBit(int &nCollectedCycles)
{

	CycleDecoder::CycleSample first_cycle_sample, cycle_sample;
	

	// Get one F2 cycle
	// If error correction is enabled, accept the first cycle to be of the wrong type (F2)
	if (!mCycleDecoder.getNextCycle(first_cycle_sample) || (!mErrorCorrection && first_cycle_sample.freq != Frequency::F2))
		return false;

	// Normally we shall see mStopBitCycles-1 F2 cycles here (from byte to byte)
	// But if it is the last byte is ssems often to be once cycle less and to allow even loss of carrier at end of last byte,
	// we allow any number of cycles (even zero).
	if (!mCycleDecoder.collectCycles(Frequency::F2, cycle_sample, mStopBitCycles-1, nCollectedCycles)) {
		return false;
	}
	nCollectedCycles += 1; // Add the first cycle already sampled

	// Check that the stop bit has the correct no of F2 cycles
	if (nCollectedCycles != mStopBitCycles)
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "%d cycles were collected (with a detected next cycle of %s) for the stop bit when expecting %d cycles\n",
				nCollectedCycles, _FREQUENCY(mCycleDecoder.getCycle().freq), mStopBitCycles
			);

	return true;
}

/*
 * Read a byte with bits in little endian order
*/
bool BlockDecoder::getByte(Byte  *byte, int &nCollectedCycles)
{
	if (!getStartBit()) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read start bit%s\n", "");
		return false;
	}
	*byte = 0;
	uint16_t f = 1;
	for (int i = 0; i < 8; i++) {
		Bit bit;
		if (!getDataBit(bit)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to read data bit b%d\n", i);
			return false;
		}
		*byte = *byte  + bit * f;
		f = f * 2;
	}

	if (!getStopBit(nCollectedCycles)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read stop bit%s\n", "")
		return false;
	}

	nReadBytes++;

	return true;
}

bool BlockDecoder::getByte(Byte* byte)
{
	int collected_cycles;

	return getByte(byte, collected_cycles);

}
