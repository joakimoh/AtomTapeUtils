#pragma once

#ifndef BLOCK_DECODER_H
#define BLOCK_DECODER_H

#include <string>

#include "../shared/CommonTypes.h"
#include "../shared/BlockTypes.h"
#include "../shared/WaveSampleTypes.h"
#include "CycleDecoder.h"





class BlockDecoder
{

private:

	CycleDecoder& mCycleDecoder;

	AtomTapeBlockHdr mAtomTapeBlockHdr; // read Atom tape header (excluding synchronisation bytes and filename)


	int mStartBitCycles;	// #cycles for start bit - should be 1 "bit"
	int mLowDataBitCycles; // #cycles for LOW "0" data bit - for F2 frequency
	int mHighDataBitCycles;	//#cycles for HIGH "1" data bit - for F1 frequency
	int mStopBitCycles;		// #cycles for stop bit - should be 1 1/2 "bit"

	ArgParser mArgParser;

	bool mErrorCorrection = false; // Apply error correction of different kinds

	bool mTracing;

	bool mVerbose = false;

	bool mReadingCRC = false;


public:

	int nReadBytes;

	BlockDecoder(CycleDecoder& cycleDecoder, ArgParser & argParser, bool verbose);

	bool readBlock(
		double leadToneDuration, ATMBlock &readBlock, BlockType &block_type, 
		int &blockNo, bool &leadToneDetected);

	double getTime();

	// Save the current file position
	bool checkpoint();

	// Roll back to a previously saved file position
	bool rollback();


private:

	// Detect a start bit by looking for exactly mStartBitCycles low tone (F1) cycles
	bool getStartBit();

	bool getDataBit(Bit& bit);

	//
	// Detect a stop bit by looking for at least one and maximum mStopBitCycles high tone (F2) cycles
	// (Even if there should normally be mStopBitCycles cycles)
	//
	bool getStopBit(int &nCollectedCycles);

	// Get one byte
	bool getByte(Byte *byte, int & nCollectedCycles);
	bool getByte(Byte* byte);

	bool checkByte(Byte refValue, Byte &readVal);

	bool checkBytes(Byte refVal, int n);

	bool getFileName(char name[13], Byte &CRC, int &len);

	// Get bytes
	bool getBytes(Bytes& block, int n, Byte &CRC);


};

#endif