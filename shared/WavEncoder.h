#pragma once
#ifndef WAV_ENCODER_H
#define WAV_ENCODER_H



#include <vector>
#include <string>
#include "TAPCodec.h"
#include "WaveSampleTypes.h"
#include "../shared/TapeProperties.h"


using namespace std;

class WavEncoder
{

private:

	Samples mSamples;
	TapeProperties mTapeTiming;
	bool mUseOriginalTiming = false;

	int mMaxSampleAmplitude = 16384;
	int mFS;
	double mHighSamples;
	double mLowSamples;;

	int mStartBitCycles;	// #cycles for start bit - should be 1 "bit"
	int mLowDataBitCycles; // #cycles for LOW "0" data bit - for F2 frequency
	int mHighDataBitCycles;	//#cycles for HIGH "1" data bit - for F1 frequency
	int mStopBitCycles;		// #cycles for stop bit - should be 1 1/2 "bit"

	Word mCRC;

	int mPhase = 180;

	bool mVerbose = false;

	TargetMachine mTargetMachine = ACORN_ATOM;

public:


	WavEncoder(int sampleFreq, bool verbose, TargetMachine targetMachine);

	WavEncoder(bool useOriginalTiming, int sampleFreq, bool verbose, TargetMachine targetMachine);

	bool setTapeTiming(TapeProperties tapeTiming);

	bool writeByte(Byte byte);
	bool writeDataBit(int bit);
	bool writeStartBit();
	bool writeStopBit();
	bool writeCycle(bool high, unsigned n);
	bool writeTone(double duration);
	bool writeGap(double duration);


	/*
	 * Encode TAP File structure as WAV file
	 */
	bool encode(TapeFile& tapeFile, string& filePath);
	bool encodeAtom(TapeFile& tapeFile, string& filePath);
	bool encodeBBM(TapeFile& tapeFile, string& filePath);



private:

	bool init();
	
};

#endif // ! WAV_ENCODER_H
