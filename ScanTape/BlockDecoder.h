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


public:

	int nReadBytes;

	BlockDecoder(CycleDecoder& cycleDecoder, ArgParser & argParser);

	bool readBlock(
		double leadToneDuration, double trailerToneDuration, ATMBlock &readBlock, BlockType &block_type, 
		int &blockNo, bool &leadToneDetected);

	double getTimeNum();

	string getTime();

	// Save the current file position
	bool checkpoint();

	// Roll back to a previously saved file position
	bool rollback();


private:

	bool getStartBit();

	bool getDataBit(Bit& bit);

	bool getStopBit(int &nCollectedCycles);

	bool getByte(Byte *byte, int & nCollectedCycles);
	bool getByte(Byte* byte);

	bool checkByte(Byte refValue);

	bool checkBytes(Byte refVal, int n);

	bool getFileName(char name[13], Byte &CRC, int &len);

	bool getBytes(Bytes& block, int n, Byte &CRC);


};

#endif