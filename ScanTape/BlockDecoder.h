#pragma once

#ifndef BLOCK_DECODER_H
#define BLOCK_DECODER_H

#include <string>

#include "../shared/CommonTypes.h"
#include "../shared/AtomBlockTypes.h"
#include "../shared/WaveSampleTypes.h"
#include "CycleDecoder.h"
#include "../shared/FileBlock.h"
#include "WavTapeReader.h"





class BlockDecoder
{

protected:

	TapeReader &mReader;

	ArgParser mArgParser;

	bool mTracing;

	bool mVerbose = false;

	TargetMachine mTargetMachine = ACORN_ATOM;

public:


	int nReadBytes;

	BlockDecoder(TapeReader& tapeReader, ArgParser& argParser);

	bool readBlock(BlockTiming blockTiming, bool firstBlock, FileBlock& readBlock, bool& leadToneDetected);

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



};

#endif