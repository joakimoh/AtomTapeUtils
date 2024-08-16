#include "BBMBlockDecoder.h"
#include "../shared/WaveSampleTypes.h"
#include <iostream>
#include "../shared/Debug.h"
#include "../shared/Utility.h"

//
// CRC is XMODEM
// - 16-bit
// - polynom x^12 + x^5 + x (1021 <=> 0001 0000 0010 0001)
// - init value 0x0
// - check value 0x31c3
//
void BBMBlockDecoder::updateCRC(uint16_t &CRC, Byte data)
{
	updateBBMCRC(CRC, data);
}

string BBMBlockDecoder::bbmTapeBlockHdrFieldName(int offset)
{
	switch (offset) {
	case 0: return "loadAdrByte0";
	case 1: return "loadAdrByte1";
	case 2: return "loadAdrByte2";
	case 3: return "loadAdrByte3";
	case 4: return "execAdrByte0";
	case 5: return "execAdrByte1";
	case 6: return "execAdrByte2";
	case 7: return "execAdrByte3";
	case 8: return "blockNoLSB";
	case 9: return "blockNoMSB";
	case 10: return "blockLenLSB";
	case 11: return "blockLenMSB";
	case 12: return "blockFlag";
	case 13: return "nextLoadAdrByte0";
	case 14: return "nextLoadAdrByte1";
	case 15: return "nextLoadAdrByte2";
	case 16: return "nextLoadAdrByte3";
	case 17: return "calc_hdr_CRC";
	default: return "???";
	}

}


BBMBlockDecoder::BBMBlockDecoder(
	CycleDecoder& cycleDecoder, ArgParser& argParser, bool verbose) : BlockDecoder(cycleDecoder, argParser, verbose)
{
}


/*
 *
 * Read a complete BBC Micro tape block, starting with the detection of its lead tone and
 * ending with the reading of the stop bit of the calc_hdr_CRC byte that ends the block.
 *
 */
bool BBMBlockDecoder::readBlock(
	double leadToneDuration, FileBlock& readBlock,
	bool &isLastBlock, int& blockNo, bool& leadToneDetected
)
{
	uint16_t calc_hdr_CRC, calc_data_CRC;
	nReadBytes = 0;
	mTapeFileName = "???";

	if (mVerbose)
		cout << "\n\nReading a new block...\n";

	// Invalidate output variables in case readBlock returns prematurely
	isLastBlock = false;
	blockNo = -1;
	leadToneDetected = false;

	// Invalidate block data in case readBlock returns with only partilly filled out block
	for (int i = 0; i < 4; i++) {
		readBlock.bbmHdr.loadAdr[i] = 0xff;
		readBlock.bbmHdr.execAdr[i] = 0xff;
	}
	for (int i = 0; i < 2; i++) {
		readBlock.bbmHdr.blockNo[i] = 0xff;
		readBlock.bbmHdr.blockLen[i] = 0xff;
	}
	readBlock.bbmHdr.locked = false;
	readBlock.bbmHdr.name[0] = 0xff;


	// Wait for lead tone of a min duration but still 'consume' the complete tone
	double duration, dummy;
	if (!mCycleDecoder.waitForTone(leadToneDuration, duration, dummy, readBlock.leadToneCycles, mLastHalfCycleFrequency)) {
		// This is not necessarily an error - it could be because the end of the tape as been reached...
			return false;
	}
	leadToneDetected = true;
	if (mVerbose)
		cout << duration << "s lead tone detected at " << encodeTime(getTime()) << "\n";

	// Read block header's preamble (i.e., synhronisation byte)
	if (!checkBytes(0x2a, 1)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read header preamble (0x2a byte)%s\n", "");
		return false;
	}

	// Get phaseshift when transitioning from lead tone to start bit.
	// (As recorded by the Cycle Decoder when reading the preamble.)
	readBlock.phaseShift = mCycleDecoder.getPhaseShift();

	// Initialize header calc_hdr_CRC
	calc_hdr_CRC = 0;

	// Read block header's name (1 to 10 characters terminated by 0x0) field
	int name_len;
	if (!getFileName(readBlock.bbmHdr.name, calc_hdr_CRC, name_len)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read header block name%s\n", "");
		return false;
	}

	// Remember tape file name (for output of debug/error messages)
	mTapeFileName = readBlock.bbmHdr.name;

	// Read BBC Micro tape header bytes
	Byte* hdr = (Byte*)&mBBMTapeBlockHdr;
	int collected_stop_bit_cycles;
	for (int i = 0; i < sizeof(mBBMTapeBlockHdr); i++) {
		if (!getByte(hdr, collected_stop_bit_cycles)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to read header %s\n", bbmTapeBlockHdrFieldName(i).c_str());
			return false;
		}
		updateCRC(calc_hdr_CRC, *hdr++);
	}

	// Extract address information and update BBM block with it
	for (int i = 0; i < 4; i++) {
		readBlock.bbmHdr.loadAdr[i] = mBBMTapeBlockHdr.loadAdr[i];
		readBlock.bbmHdr.execAdr[i] = mBBMTapeBlockHdr.execAdr[i];
	}
	for (int i = 0; i < 2; i++) {
		readBlock.bbmHdr.blockNo[i] = mBBMTapeBlockHdr.blockNo[i];
		readBlock.bbmHdr.blockLen[i] = mBBMTapeBlockHdr.blockLen[i];
	}
	readBlock.bbmHdr.locked = (mBBMTapeBlockHdr.blockFlag & 0x1) != 0;

	// Extract block no (returned to caller of readBlock)
	blockNo = mBBMTapeBlockHdr.blockNo[0] * 256 + mBBMTapeBlockHdr.blockNo[1];

	// Extract data length information
	int len = readBlock.bbmHdr.blockLen[0] * 256 + readBlock.bbmHdr.blockLen[1];

	// Interpret block flag (b7 = last block - ignored, b6 = block is empty - ignored, b0 = file locked - considered)
	if (mBBMTapeBlockHdr.blockFlag & 0x1)
		readBlock.bbmHdr.locked = true;
	else
		readBlock.bbmHdr.locked = false;
	if (mBBMTapeBlockHdr.blockFlag & 0x80)
		isLastBlock = true;
	else
		isLastBlock = false;

	if (mVerbose)
		cout << "Header " << mTapeFileName << " " << hex <<
		((mBBMTapeBlockHdr.loadAdr[0] >> 24) & 0xff) <<
		((mBBMTapeBlockHdr.loadAdr[1] >> 16) & 0xff) <<
		((mBBMTapeBlockHdr.loadAdr[2] >> 8) & 0xff) <<
		((mBBMTapeBlockHdr.loadAdr[3]) & 0xff) <<
		" " <<
		((mBBMTapeBlockHdr.blockLen[0] >> 8) & 0xff) <<
		((mBBMTapeBlockHdr.blockLen[1]) & 0xff) <<
		" " <<
		((mBBMTapeBlockHdr.blockNo[0] >> 8) & 0xff) <<
		((mBBMTapeBlockHdr.blockNo[1]) & 0xff) <<
		" read at " << encodeTime(getTime()) << dec << "\n";

	// Get header CRC
	Word hdr_CRC;
	if (!getWord(&hdr_CRC)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read header CRC for file '%s'\n", readBlock.bbmHdr.name);
		return false;
	}

	if (mVerbose)
		cout << "header CRC read at " << encodeTime(getTime()) << "\n";

	// Check calc_hdr_CRC
	if (false && hdr_CRC != calc_hdr_CRC) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Calculated header CRC 0x%x differs from received data CRC 0x%x for file '%s'\n", calc_hdr_CRC, hdr_CRC, readBlock.bbmHdr.name);
		return false;
	}


	// Consume micro lead tone between header and data block (to record it's duration only)
	int micro_tone_half_cycles;
	if (!mCycleDecoder.stopOnHalfCycles(Frequency::F2, 1, dummy, mLastHalfCycleFrequency)) { // Find the  first F2 1/2 cycle
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to detect a start of data tone for file '%s'\n", readBlock.bbmHdr.name);
		return false;
	}
	if (!mCycleDecoder.consumeHalfCycles(Frequency::F2, micro_tone_half_cycles, mLastHalfCycleFrequency)) { // Consume the remaining 1/2 cycles
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to detect a start of data tone for file '%s'\n", readBlock.bbmHdr.name);
		return false;
	}
	micro_tone_half_cycles++;
	double micro_tone_duration = (double)micro_tone_half_cycles / (F2_FREQ * 2);
	mArgParser.tapeTiming.minBlockTiming.microLeadToneDuration = micro_tone_duration;

	if (mVerbose)
		cout << micro_tone_duration << "s micro tone detected at " << encodeTime(getTime()) << "\n";

	// Get data bytes
	calc_data_CRC = 0x0;
	if (len > 0) {
		if (!getBytes(readBlock.data, len, calc_data_CRC)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to read block data for file '%s'!\n", readBlock.bbmHdr.name);
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

	// Get data CRC
	Word data_CRC;
	if (!getWord(&data_CRC)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read data block CRC for file '%s'\n", readBlock.bbmHdr.name);
		return false;
	}

	if (mVerbose)
		cout << "data CRC read at " << encodeTime(getTime()) << "\n";

	// Check calc_hdr_CRC
	if (data_CRC != calc_data_CRC) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Calculated data CRC 0x%x differs from received data CRC 0x%x for file '%s'\n", calc_data_CRC, data_CRC, readBlock.bbmHdr.name);
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


bool BBMBlockDecoder::getFileName(char name[MAX_BBM_NAME_LEN], uint16_t& calc_hdr_CRC, int& len) {
	int i = 0;
	Byte byte;
	bool failed = false;
	len = 0;
	do {
		if (!getByte(&byte)) {
			return false;
		}
		else if (byte != 0x0)
			name[i] = (char)byte;
		else
			len = i;
		i++;
		updateCRC(calc_hdr_CRC,byte);
	} while (!failed && byte != 0x0 && i <= MAX_BBM_NAME_LEN);

	// Add zero-padding
	for (int j = len; j < MAX_BBM_NAME_LEN; j++)
		name[j] = '\0';

	return true;

}
