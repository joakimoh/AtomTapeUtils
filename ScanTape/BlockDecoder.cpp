#include "BlockDecoder.h"
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
}

BlockDecoder::BlockDecoder(CycleDecoder& cycleDecoder, ArgParser &argParser) : mCycleDecoder(cycleDecoder), mArgParser(argParser)
{

	mTracing = argParser.tracing;

	if (mArgParser.mBaudRate == 300) {
		mStartBitCycles = 4; // Start bit length in cycles of F1 frequency carrier
		mLowDataBitCycles = 4; // Data bit length in cycles of F1 frequency carrier
		mHighDataBitCycles = 8; // Data bit length in cycles of F2 frequency carrier
		mStopBitCycles = 9; // Stop bit length in cycles of F2 frequency carrier
	}
	else if (mArgParser.mBaudRate == 1200) {
		mStartBitCycles = 1; // Start bit length in cycles of F1 frequency carrier
		mLowDataBitCycles = 1; // Data bit length in cycles of F1 frequency carrier
		mHighDataBitCycles = 2; // Data bit length in cycles of F2 frequency carrier
		mStopBitCycles = 3; // Stop bit length in cycles of F2 frequency carrier
	}
	else {
		throw invalid_argument("Unsupported baud rate " + mArgParser.mBaudRate);
	}

	if (argParser.mErrorCorrection)
		mErrorCorrection = true;
}


/*
 * 
 * Read a complete Acorn Atom tape block, starting with the detection of its lead tone and
 * ending with the reading of the stop bit of the CRC byte that ends the block.
 * 
 *
 * A first block starts with 5.1 seconds  of a 2400 Hz lead tone.
 * 
 * Before the next block there seems to be 2 seconds of silence, noise or mis-shaped waves that
 * are neither 1200 nor 2400 cycles.
 * 
 * The second and all subsequent blocks belonging to the same file starts with 2 seconds of a 2400
 * lead tone (i.e., different than the 5.1 seconds for the first block).
 * 
 * Within each block there seems to be a 0.5 seconds micro lead tone between the block header and the block data.
 * It starts with a bit shorter stop bit (8 cycles of 2400 Hz instead of 9 cycles as is normally the case) followed by
 * 0.5 sec of a 2400 tone before the data block start bit comes.
 * 
 * Between two files, there seems to up to 2.5 seconds of silence, noise or malformed waves that
 * are neither 1200 nor 2400 cycles.
 * 
 * Sometimes there could be gaps between bytes in a data block with seconds of a 2400 tone inbetween.
 * 
 * Contrary to what is stated on beebwiki, there seems to be no trailer tone whatsoever ending a block or a file.
 * The lead tone for subsequent block are also longer (2 sec) than what beewiki states (0.9/1.1 sec).
 * 
 * Beebwiki states:
 * 
 * "Each data stream is preceded by 5.1 seconds of 2400 Hz lead tone (reduced to 1.1 seconds
 * if the recording has paused in the middle of a file, or 0.9 seconds between data blocks recorded in one go).
 * At the end of the stream is a 5.3 second, 2400 Hz trailer tone (reduced to 0.2 seconds when
 * pausing in the middle of a file (giving at least 1.3 seconds' delay between data blocks.)"
 *
 */
bool BlockDecoder::readBlock(
	double leadToneDuration, double trailerToneDuration, ATMBlock &readBlock,
	BlockType &block_type, int& blockNo, bool &leadToneDetected
)
{
	Byte CRC;
	nReadBytes = 0;

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
	double duration;
	if (!mCycleDecoder.waitForTone(leadToneDuration, duration)) {
		// This is not necessariyl an error - it could be because the end of the tape as been reached...
		return false;
	}
	leadToneDetected = true;

	// Read block header's preamble (i.e., synhronisation bytes)
	if (!checkBytes(0x2a, 4)) {
		if (mTracing)
			DEBUG_PRINT(getTimeNum(), ERR, "Failed to read header preamble (0x2a bytes)\n");
		return false;
	}

	// Initialize CRC with sum of preamble
	CRC = (Byte) (0x2a * 4);

	// Read block header's name (1 to 13 characters + 0xd terminator) field
	int name_len;
	if (!getFileName(readBlock.hdr.name, CRC, name_len)) {
		if (mTracing)
			DEBUG_PRINT(getTimeNum(), ERR, "Failed to read header block name\n");
		return false;
	}

	// Read Atom tape header bytes
	Byte * hdr = (Byte*)&mAtomTapeBlockHdr;
	int collected_stop_bit_cycles;
	for (int i = 0; i < sizeof(mAtomTapeBlockHdr); i++) {
		if (!getByte(hdr, collected_stop_bit_cycles)) {
			if (mTracing)
				DEBUG_PRINT(getTimeNum(), ERR, "Failed to read header %s\n", atomTapeBlockHdrFieldName(i).c_str());
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
	
	// Get data bytes
	if (collected_stop_bit_cycles < mStopBitCycles) {
		if (!mCycleDecoder.waitForTone(mArgParser.mMinMicroLeadTone, duration)) {
			if (mTracing)
				DEBUG_PRINT(getTimeNum(), ERR, "Failed to detect a start of data tone for file '%s'\n", readBlock.hdr.name);
		}
	}

	if (len > 0) {
		if (!getBytes(readBlock.data, len, CRC)) {
			if (mTracing)
				DEBUG_PRINT(getTimeNum(), ERR, "Failed to read data!\n");
			return false;
		}
	}

	// Get CRC
	uint8_t received_CRC = 0;
	if (!getByte(&received_CRC, collected_stop_bit_cycles)) {
		if (mTracing)
			DEBUG_PRINT(getTimeNum(), ERR, "Failed to read CRC for file '%s'\n", readBlock.hdr.name);
		return false;
	}

	// Check CRC
	if (received_CRC != CRC) {
		if (mTracing)
			DEBUG_PRINT(getTimeNum(), ERR, "Calculated CRC 0x%x differs from received CRC 0x%x for file '%s'\n", CRC, received_CRC, readBlock.hdr.name);
		return false;
	}

	if (collected_stop_bit_cycles == mStopBitCycles) {
		if (!mCycleDecoder.waitForTone(trailerToneDuration, duration)) {
			if (mTracing)
				DEBUG_PRINT(getTimeNum(), ERR, "Failed to detect a trailer tone for file '%s'\n", readBlock.hdr.name);
			return false;
		}
	}


	return true;
}

bool BlockDecoder::checkByte(Byte refVal) {
	Byte byte = 0x11;
	if (!getByte(&byte) || byte != refVal) {
		return false;
	}
	return true;
}



bool BlockDecoder::checkBytes(Byte refVal, int n) {
	bool failed = false;
	int start_cycle = 0;
	for (int i = start_cycle; i < n && !failed; i++) {
		if (!checkByte(refVal)) {
			if (mTracing)
				DEBUG_PRINT(getTimeNum(), ERR, "Byte #%d out of %d bytes differs from reference value 0x%.2x or couldn't be read!\n", i, n, refVal);
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
		if (!getByte(&byte))
			return false;
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
				DEBUG_PRINT(getTimeNum(), ERR, "Failed to read byte #%d out of %d bytes\n", i, n);
			failed = true;
		}
		CRC += byte;
		bytes.push_back(byte);
	}
	return !failed;
}

bool BlockDecoder::getStartBit()
{

	CycleDecoder::CycleSample first_cycle_sample = mCycleDecoder.getCycle();

	// One cycle of the start bit has already been sampled when reading the stop bit or a lead tone - check that it was an F1 cycle
	if (mCycleDecoder.getCycle().freq != CycleDecoder::Frequency::F1) {
		if (mTracing)
			DEBUG_PRINT(getTimeNum(), ERR, "start bit cycle already sampled was not an F1 (but an %s)\n", _FREQUENCY(mCycleDecoder.getCycle().freq));
		return false;
	}

	CycleDecoder::CycleSample last_cycle_sample = mCycleDecoder.getCycle();

	// Collect the remaining cycles and check that they also where of type F1
	int n_collected_cycles;
	if (!mCycleDecoder.collectCycles(CycleDecoder::Frequency::F1, mStartBitCycles - 1, last_cycle_sample, n_collected_cycles)) {
		if (mTracing)
			DEBUG_PRINT(getTimeNum(), ERR, "Failed when collecting F1 cycles - last sampled cycle was %s\n", _FREQUENCY(last_cycle_sample.freq));
		return false;
	}

	return true;
}

string BlockDecoder::getTime() {
	double t = mCycleDecoder.getCycle().sampleEnd * T_S;
	char t_str[64];
	int t_h = (int) trunc(t / 3600);
	int t_m = (int) trunc((t - t_h) / 60);
	double t_s = t - t_h * 3600 - t_m * 60;
	sprintf_s(t_str, "%dh:%dm:%.6fs (%fs)", t_h, t_m, t_s, t);
	return string(t_str);
}

double BlockDecoder::getTimeNum() {
	return mCycleDecoder.getCycle().sampleEnd * T_S;
}

bool BlockDecoder::getDataBit(Bit& bit)
{

	CycleDecoder::CycleSample first_cycle_sample, last_cycle_sample;

	// Get one cycle of either F1 or F2 type to determine how many more cycles to read
	if (!mCycleDecoder.getNextCycle(first_cycle_sample) || !(first_cycle_sample.freq == CycleDecoder::Frequency::F1 || first_cycle_sample.freq == CycleDecoder::Frequency::F2))
	{
		if (mTracing)
			DEBUG_PRINT(getTimeNum(), ERR, "First cycle of data bit was an illegal %s cycle\n", _FREQUENCY(first_cycle_sample.freq));
		return false;
	}
	
	if (first_cycle_sample.freq == CycleDecoder::Frequency::F1) {
		int n_collected_cycles;
		if (!mCycleDecoder.collectCycles(CycleDecoder::Frequency::F1, mLowDataBitCycles - 1, last_cycle_sample, n_collected_cycles)) {
			if (mTracing)
				DEBUG_PRINT(getTimeNum(), ERR, "No of F1 Cycles for databit was %d when expecting %d\n", n_collected_cycles + 1, mLowDataBitCycles);
			return false;
		}
		// A "0" bit
		bit = Low;
	}
	else if (first_cycle_sample.freq == CycleDecoder::Frequency::F2) {
		int n_collected_cycles;
		if (!mCycleDecoder.collectCycles(CycleDecoder::Frequency::F2, mHighDataBitCycles - 1, last_cycle_sample, n_collected_cycles)) {
			// If error correction is enabled, then accept 1 lost F2 cycle
			if (!mErrorCorrection || n_collected_cycles < mHighDataBitCycles - 1) {
				if (mTracing)
					DEBUG_PRINT(getTimeNum(), ERR, "No of F2 Cycles for databit was %d when expecting %d\n", n_collected_cycles + 1, mHighDataBitCycles);
				return false;
			}
		}
		// An "1" bit
		bit = High;
	}
	else {
		// Not a valid data bit
		return false;
	}

	return true;

}

bool BlockDecoder::getStopBit(int &nCollectedCycles)
{

	CycleDecoder::CycleSample first_cycle_sample, cycle_sample;
	

	// Get one F2 cycle
	// If error correction is enabled, accept the first cycle to be of the wrong type (F2)
	if (!mCycleDecoder.getNextCycle(first_cycle_sample) || (!mErrorCorrection && first_cycle_sample.freq != CycleDecoder::Frequency::F2))
		return false;

	// Normally we shall se mStopBitCycles-1 F2 cycles here (from byte to byte)
	// But if it is the last byte before a trailer tone or to allow even loss of carrier as end of last byte,
	// we allow zero up to mStopBitCycles-1 F2 cycles...
	if (!mCycleDecoder.collectCycles(CycleDecoder::Frequency::F2, cycle_sample, mStopBitCycles-1, nCollectedCycles)) {
		return false;
	}
	if (nCollectedCycles != mStopBitCycles - 1)
		if (mTracing)
			DEBUG_PRINT(getTimeNum(), ERR, "%d cycles were collected (with a detected next cycle of %s) for the stop bit when expecting %d cycles\n",
				nCollectedCycles + 1, _FREQUENCY(mCycleDecoder.getCycle().freq), mStopBitCycles
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
			DEBUG_PRINT(getTimeNum(), ERR, "Failed to read start bit\n");
		return false;
	}
	*byte = 0;
	uint16_t f = 1;
	for (int i = 0; i < 8; i++) {
		Bit bit;
		if (!getDataBit(bit)) {
			if (mTracing)
				DEBUG_PRINT(getTimeNum(), ERR, "Failed to read data bit b%d\n", i);
			return false;
		}
		*byte = *byte  + bit * f;
		f = f * 2;
	}

	if (!getStopBit(nCollectedCycles)) {
		if (mTracing)
			DEBUG_PRINT(getTimeNum(), ERR, "Failed to read stop bit\n")
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
