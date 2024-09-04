#include "BBMBlockDecoder.h"
#include "../shared/WaveSampleTypes.h"
#include <iostream>
#include "../shared/Debug.h"
#include "../shared/Utility.h"

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
	CycleDecoder& cycleDecoder, ArgParser& argParser, bool verbose) : BlockDecoder(cycleDecoder, argParser, verbose, BBC_MODEL_B)
{
}


/*
 *
 * Read a complete BBC Micro tape block, starting with the detection of its lead tone and
 * ending with the reading of the stop bit of the (data) CRC that ends the block.
 *
 * Timing and format of a BBC Micro Tape File is
 * <0.625s lead tone prelude > <dummy byte> <0.625s lead tone postlude>] <header with CRC>
 *  {<data with CRC> <0.25s lead tone> <data with CRC } <0.83s trailer tone> <3.3s gap>
 *
 * This means that only the first block of a tape file will have a dummy byte inserted within
 * the lead tone. All blocks are separated only by an 0.25 lead tone and the last block ends
 * with a 0.83s lead tone followed by a 3.3s gap (before a new tape file starts).
 * 
 */
bool BBMBlockDecoder::readBlock(
	int preludeLadToneCycles, double leadToneDuration, double trailerToneDuration, FileBlock& readBlock, bool firstBlock,
	bool &isLastBlock, int& blockNo, bool& leadToneDetected
)
{
	uint16_t calc_hdr_CRC, calc_data_CRC;
	uint32_t load_adr, exec_adr, next_adr, block_len;

	nReadBytes = 0;
	mTapeFileName = "???";

	if (mVerbose)
		cout << "\n\nReading a new block...\n";

	// Invalidate output variables in case readBlock returns prematurely
	isLastBlock = false;
	blockNo = -1;
	leadToneDetected = false;

	// Invalidate block data in case readBlock returns with only partilly filled out block
	Utility::initTapeHdr(readBlock);


	// Wait for lead tone of a min duration but still 'consume' the complete tone (including dummy byte if applicable)
	double duration, waiting_time;
	if (firstBlock) {

		double prelude_tone_duration = (double) preludeLadToneCycles / F2_FREQ;
		if (!mCycleDecoder.waitForTone(prelude_tone_duration, duration, waiting_time, readBlock.preludeToneCycles, mLastHalfCycleFrequency)) {
			// This is not necessarily an error - it could be because the end of the tape as been reached...
			return false;
		}
		if (mVerbose)
			cout << duration << "s prelude lead tone detected at " << Utility::encodeTime(getTime()) << "\n";

		// Skip dummy byte
		if (!checkBytes(0xaa, 1)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to read dummy byte (0xaa byte)%s\n", "");
			//return false;
		}
		if (mVerbose)
			cout << "dummy byte 0xaa (as part of lead carrier) detected at " << Utility::encodeTime(getTime()) << "\n";

		// Get postlude part of lead tone
		if (!mCycleDecoder.waitForTone(leadToneDuration, duration, waiting_time, readBlock.leadToneCycles, mLastHalfCycleFrequency)) {
			// This is not necessarily an error - it could be because the end of the tape as been reached...
			return false;
		}
		if (mVerbose)
			cout << duration << "s postlude lead tone detected at " << Utility::encodeTime(getTime()) << "\n";
	}
	else {
		if (!mCycleDecoder.waitForTone(leadToneDuration, duration, waiting_time, readBlock.leadToneCycles, mLastHalfCycleFrequency)) {
			// This is not necessarily an error - it could be because the end of the tape as been reached...
			return false;
		}
		if (mVerbose)
			cout << duration << "s lead tone detected at " << Utility::encodeTime(getTime()) << "\n";
	}
	leadToneDetected = true;

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
		Utility::updateCRC(BBC_MODEL_B, calc_hdr_CRC, *hdr++);
	}

	// Extract header information and update BBM block with it
	if (!Utility::decodeBBMTapeHdr(mBBMTapeBlockHdr, readBlock, load_adr, exec_adr, block_len, blockNo, next_adr, isLastBlock))
		return false;


	if (mVerbose) {
		cout << "Header read at " << Utility::encodeTime(getTime()) << ": ";
		Utility::logTAPBlockHdr(readBlock, 0x0);
	}

	// Get header CRC
	Word hdr_CRC;
	if (!getWord(&hdr_CRC)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read header CRC for file '%s'\n", readBlock.bbmHdr.name);
		return false;
	}

	
	// Check calc_hdr_CRC
	if (false && hdr_CRC != calc_hdr_CRC) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Calculated header CRC 0x%x differs from received data CRC 0x%x for file '%s'\n", calc_hdr_CRC, hdr_CRC, readBlock.bbmHdr.name);
		return false;
	}

	if (mVerbose)
		cout << "A correct header CRC 0x" << hex << hdr_CRC << " read at " << Utility::encodeTime(getTime()) << "\n";


	// Get data bytes
	calc_data_CRC = 0x0;
	if (block_len > 0) {
		if (!getBytes(readBlock.data, block_len, calc_data_CRC)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to read block data for file '%s'!\n", readBlock.bbmHdr.name);
			return false;
		}
	}

	if (mVerbose)
		cout << dec << block_len << " bytes of data read at " << Utility::encodeTime(getTime()) << "\n";


	if (DEBUG_LEVEL == DBG)
		Utility::logData(load_adr, &readBlock.data[0], block_len);

	// Get data CRC
	Word data_CRC;
	if (!getWord(&data_CRC)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read data block CRC for file '%s'\n", readBlock.bbmHdr.name);
		return false;
	}
	
	// Check calc_hdr_CRC
	if (data_CRC != calc_data_CRC) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Calculated data CRC 0x%x differs from received data CRC 0x%x for file '%s'\n", calc_data_CRC, data_CRC, readBlock.bbmHdr.name);
		return false;
	}

	if (mVerbose)
		cout << "A correct data CRC 0x" << hex << data_CRC << " read at " << Utility::encodeTime(getTime()) << "\n";


	// Skip trailer tone that only exists for the last block of a file
	if (isLastBlock) {
		double duration, dummy;
		if (!mCycleDecoder.waitForTone(trailerToneDuration, duration, dummy, readBlock.trailerToneCycles, mLastHalfCycleFrequency)) {
			// This is not necessarily an error - it could be because the end of the tape as been reached...
			return false;
		}

		if (mVerbose)
			cout << duration << "s trailer tone detected at " << Utility::encodeTime(getTime()) << "\n";


		// Detect gap to next file by waiting for 100 cycles of the next files's first block's lead tone
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
	}

	return true;
}


bool BBMBlockDecoder::getFileName(char name[BTM_HDR_NAM_SZ], uint16_t& calc_hdr_CRC, int& len) {
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
		Utility::updateCRC(mTargetMachine, calc_hdr_CRC,byte);
	} while (!failed && byte != 0x0 && i <= BBM_TAPE_NAME_LEN);

	// Add zero-padding
	for (int j = len; j < BBM_TAPE_NAME_LEN; j++)
		name[j] = '\0';

	return true;

}
