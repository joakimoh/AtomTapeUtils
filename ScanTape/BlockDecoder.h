#pragma once

#ifndef BLOCK_DECODER_H
#define BLOCK_DECODER_H

#include <string>

#include "../shared/CommonTypes.h"
#include "../shared/AtomBlockTypes.h"
#include "../shared/WaveSampleTypes.h"
#include "CycleDecoder.h"
#include "../shared/BlockTypes.h"





class BlockDecoder
{

protected:

	CycleDecoder& mCycleDecoder;

	int mStartBitCycles;	// #cycles for start bit - should be 1 "bit"
	int mLowDataBitCycles; // #cycles for LOW "0" data bit - for F2 frequency
	int mHighDataBitCycles;	//#cycles for HIGH "1" data bit - for F1 frequency
	int mStopBitCycles;		// #cycles for stop bit - should be 1 1/2 "bit"
	int mDataBitSamples; // duration (in samples) of a data bit
	int mDataBitHalfCycleBitThreshold; // threshold (in 1/2 cycles) between a '0' and a '1'

	Frequency mLastHalfCycleFrequency = Frequency::UndefinedFrequency; // no of samples in last read 1/2 cycle

	ArgParser mArgParser;

	bool mErrorCorrection = false; // Apply error correction of different kinds

	bool mTracing;

	bool mVerbose = false;

	bool mReadingCRC = false;

	string mTapeFileName = "???";

	TargetMachine mTargetMachine = ACORN_ATOM;


public:

	int nReadBytes = 0;

	BlockDecoder(CycleDecoder& cycleDecoder, ArgParser& argParser, bool verbose, TargetMachine targetMachine);

	double getTime();

	// Save the current file position
	bool checkpoint();

	// Roll back to a previously saved file position
	bool rollback();

protected:

	// Detect a start bit by looking for exactly mStartBitCycles low tone (F1) cycles
	bool getStartBit();

	bool getDataBit(Bit& bit);

	bool getStopBit();

	// Get one byte
	bool getByte(Byte *byte, int & nCollectedCycles);
	bool getByte(Byte* byte);

	// Get a word (two bytes)
	bool getWord(Word* word);

	bool checkByte(Byte refValue, Byte &readVal);

	bool checkBytes(Byte refVal, int n);

	// Get bytes
	bool getBytes(Bytes& block, int n, Word &CRC);


};

#endif