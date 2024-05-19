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
	mDataBitSamples = round(mCycleDecoder.getF2Duration() * mHighDataBitCycles); // No of samples for a data bit

	// Threshold between no of 1/2 cycles for a '0' and '1' databit
	mDataBitHalfCycleBitThreshold = mLowDataBitCycles + mHighDataBitCycles;

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
	double leadToneDuration, ATMBlock& readBlock,
	BlockType& block_type, int& blockNo, bool& leadToneDetected
)
{
	Byte CRC;
	nReadBytes = 0;
	mAtomFileName = "???";

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

	// Wait for lead tone of a min duration but still 'consume' the complete tone
	double duration, dummy;
	if (!mCycleDecoder.waitForTone(leadToneDuration, duration, dummy, readBlock.leadToneCycles, mLastHalfCycleFrequency)) {
		// This is not necessarily an error - it could be because the end of the tape as been reached...
		return false;
	}
	leadToneDetected = true;
	if (mVerbose)
		cout << duration << "s lead tone detected at " << encodeTime(getTime()) << "\n";

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
	CRC = (Byte)(0x2a * 4);

	// Read block header's name (1 to 13 characters + 0xd terminator) field
	int name_len;
	if (!getFileName(readBlock.hdr.name, CRC, name_len)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read header block name%s\n", "");
		return false;
	}

	// Remember tape file name (for output of debug/error messages)
	mAtomFileName = readBlock.hdr.name;

	// Read Atom tape header bytes
	Byte* hdr = (Byte*)&mAtomTapeBlockHdr;
	int collected_stop_bit_cycles;
	for (int i = 0; i < sizeof(mAtomTapeBlockHdr); i++) {
		if (!getByte(hdr, collected_stop_bit_cycles)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to read header %s\n", atomTapeBlockHdrFieldName(i).c_str());
			return false;
		}
		CRC += *hdr++;
	}


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

	if (mVerbose)
		cout << "Header " << mAtomFileName << " " << hex << mAtomTapeBlockHdr.loadAdrHigh * 256 + mAtomTapeBlockHdr.loadAdrLow << " " << mAtomTapeBlockHdr.dataLenM1 + 1 << " " << blockNo << " read at " << encodeTime(getTime()) << dec << "\n";


	// Consume micro lead tone between header and data block (to record it's duration only)
	int micro_tone_half_cycles;
	if (!mCycleDecoder.stopOnHalfCycles(Frequency::F2, 1, dummy, mLastHalfCycleFrequency)) { // Find the  first F2 1/2 cycle
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to detect a start of data tone for file '%s'\n", readBlock.hdr.name);
		return false;
	}
	if (!mCycleDecoder.consumeHalfCycles(Frequency::F2, micro_tone_half_cycles, mLastHalfCycleFrequency)) { // Consume the remaining 1/2 cycles
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to detect a start of data tone for file '%s'\n", readBlock.hdr.name);
		return false;
	}
	micro_tone_half_cycles++;
	double micro_tone_duration = (double)micro_tone_half_cycles / (F2_FREQ * 2);
	mArgParser.tapeTiming.minBlockTiming.microLeadToneDuration = micro_tone_duration;

	if (mVerbose)
		cout << micro_tone_duration << "s micro tone detected at " << encodeTime(getTime()) << "\n";

	// Get data bytes
	if (len > 0) {
		if (!getBytes(readBlock.data, len, CRC)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to read block data for file '%s'!\n", readBlock.hdr.name);
			return false;
		}
	}

	if (mVerbose) {
		cout << "Data bytes: ";
		for (int i = 0; i < len; i++)
			cout << _BYTE(readBlock.data[i]) << " ";
		cout << "\n";

		cout << len << " bytes of Block data read at " << encodeTime(getTime()) << "\n";
	}

	// Get CRC
	uint8_t received_CRC = 0;
	mReadingCRC = true;
	if (!getByte(&received_CRC, collected_stop_bit_cycles)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read CRC for file '%s'\n", readBlock.hdr.name);
		return false;
	}
	mReadingCRC = false;

	if (mVerbose)
		cout << "CRC read at " << encodeTime(getTime()) << "\n";

	// Check CRC
	if (received_CRC != CRC) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Calculated CRC 0x%x differs from received CRC 0x%x for file '%s'\n", CRC, received_CRC, readBlock.hdr.name);
		return false;
	}

	if (mVerbose) {
		cout << len << " bytes data block + CRC read at " << encodeTime(getTime()) << "\n";
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

bool BlockDecoder::getFileName(char name[ATM_MMC_HDR_NAM_SZ], Byte &CRC, int &len) {
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
	} while (!failed && byte != 0xd && i <= MAX_TAPE_NAME_LEN);

	// Add zero-padding
	for (int j = len; j < ATM_MMC_HDR_NAM_SZ; j++)
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

	// Wait for mStartBitCycles * 2 "1/2 cycles" of low frequency F1 <=> start bit
	double waiting_time;
	double t_start = getTime();
	int n_remaining_start_bit_cycles = mStartBitCycles * 2;
	if (mLastHalfCycleFrequency == Frequency::F1)
		--n_remaining_start_bit_cycles;
	if (mTracing)
		DEBUG_PRINT(getTime(), DBG, "Last 1/2 Cycle was of type %s\n", _FREQUENCY(mLastHalfCycleFrequency));
	if (!mCycleDecoder.stopOnHalfCycles(Frequency::F1, n_remaining_start_bit_cycles, waiting_time, mLastHalfCycleFrequency)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to detect start bit\n", "");
		return false;
	}

	return true;
}


double BlockDecoder::getTime() {
	return mCycleDecoder.getTime();
}

bool BlockDecoder::getDataBit(Bit& bit)
{
	int n_half_cycles;

	// Advance time corresponding to one bit and count the no of transitions (1/2 cycles)
	if (!mCycleDecoder.countHalfCycles(mDataBitSamples, n_half_cycles)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Unexpected end of samples when reading data bit\n");
		return false; // unexpected end of samples
	}

	// Decide whether the databit was a '0' or a '1' value based on the no of detected 1/2 cycles
	if (n_half_cycles < mDataBitHalfCycleBitThreshold)
		bit = LowBit;
	else
		bit = HighBit;

	return true;

}


/*
 * Read a byte with bits in little endian order
*/
bool BlockDecoder::getByte(Byte  *byte, int &nCollectedCycles)
{
	// Detect start bit
	if (!getStartBit()) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read start bit%s\n", "");
		return false;
	}
	DEBUG_PRINT(getTime(), DBG, "Got start bit\n", "");

	// Get data bits
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

	// No need to detect stop bit as the search for a start bit in the next
	// byte will continue past the stop bit.

	nReadBytes++;

	DEBUG_PRINT(getTime(), DBG, "Got byte %.2x\n", *byte);

	return true;
}

bool BlockDecoder::getByte(Byte* byte)
{
	int collected_cycles;

	return getByte(byte, collected_cycles);

}
