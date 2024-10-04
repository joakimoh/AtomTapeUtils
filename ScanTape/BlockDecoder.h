#pragma once

#ifndef BLOCK_DECODER_H
#define BLOCK_DECODER_H

#include <string>
#include <cstdint>

#include "../shared/CommonTypes.h"
#include "../shared/AtomBlockTypes.h"
#include "../shared/WaveSampleTypes.h"
#include "CycleDecoder.h"
#include "../shared/FileBlock.h"
#include "WavTapeReader.h"
#include "ArgParser.h"



enum BlockError {
	BLOCK_OK = 0, BLOCK_DATA_CRC_ERR = 0x2, BLOCK_HDR_CRC_ERR = 0x4, BLOCK_CRC_ERR = 0x6
};

inline BlockError operator|(BlockError left, BlockError right) {
	return static_cast<BlockError>(static_cast<uint8_t>(left) | static_cast<uint8_t>(right));
}

inline BlockError operator&(BlockError left, BlockError right) {
	return static_cast<BlockError>(static_cast<uint8_t>(left) & static_cast<uint8_t>(right));
}

inline BlockError &operator|=(BlockError &left, BlockError right) {
	return left = left | right;
}

inline BlockError& operator&=(BlockError& left, BlockError right) {
	return left = left & right;
}

class BlockDecoder
{

protected:

	TapeReader &mReader;

	ArgParser mArgParser;

	bool mTracing;

	double mDbgStart = 0.0;
	double mDbgEnd = 0.0;

	bool mVerbose = false;

	TargetMachine mTargetMachine = ACORN_ATOM;

public:


	int nReadBytes;

	BlockDecoder(TapeReader& tapeReader, ArgParser& argParser);

	bool readBlock(BlockTiming blockTiming, bool firstBlock, FileBlock& readBlock, bool& leadToneDetected, BlockError &readStatus);

	// Get tape time
	double getTime() { return mReader.getTime(); }

	// Save the current file position
	bool checkpoint();

	// Roll back to a previously saved file position
	bool rollback();

protected:

	bool updateCRC(FileBlock block, Word& crc, Bytes data);

	// Get a word (two bytes)
	bool getWord(Word* word);

	bool checkByte(Byte refValue, Byte &readVal);

	bool checkBytes(Bytes &bytes, Byte refVal, int n);

	// Get block name
	bool getBlockName(Bytes& name);

	static int getMinLeadCarrierCycles(bool firstBlock, BlockTiming blockTiming, TargetMachine targetMachine, double carrierFreq);

};

#endif