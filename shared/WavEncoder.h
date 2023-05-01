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

	TAPFile mTapFile;
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

	Byte mCRC;

	int mPhase = 180;

public:


	WavEncoder(int sampleFreq);

	WavEncoder(TAPFile& tapFile, bool useOriginalTiming, int sampleFreq);

	bool setTapeTiming(TapeProperties tapeTiming);

	bool writeByte(Byte byte);
	bool writeDataBit(int bit);
	bool writeStartBit();
	bool writeStopBit();
	bool writeCycle(bool high, int n);
	bool writeTone(double duration);
	bool writeGap(double duration);


	/*
	 * Encode TAP File structure as WAV file
	 */
	bool encode(string& filePath);


	/*
	 * Get the encoder's TAP file
	 */
	bool getTAPFile(TAPFile& tapFile);

	/*
	 * Reinitialise encoder with a new TAP file
	 */
	bool setTAPFile(TAPFile& tapFile);



private:

	
};

#endif // ! WAV_ENCODER_H
