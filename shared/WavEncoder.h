#pragma once
#ifndef WAV_ENCODER_H
#define WAV_ENCODER_H



#include <vector>
#include <string>
#include "TAPCodec.h"
#include "WaveSampleTypes.h"


using namespace std;

class WavEncoder
{

private:

	TAPFile mTapFile;
	Samples mSamples;

	double mLeadToneDuration = 4.2;
	double mOtherBlockLeadToneDuration = 2.0;
	double mDataBlockMicroLeadToneDuration = 0.5;
	double mBlockGap = 2.0;
	double mLastBlock_gap = 2.5;

	int mBaudrate;
	int mMaxSampleAmplitude = 16384;
	double mHighSamples = (double) F_S / F2_FREQ;
	double mLowSamples = (double) F_S / F1_FREQ;

	int mStartBitCycles;	// #cycles for start bit - should be 1 "bit"
	int mLowDataBitCycles; // #cycles for LOW "0" data bit - for F2 frequency
	int mHighDataBitCycles;	//#cycles for HIGH "1" data bit - for F1 frequency
	int mStopBitCycles;		// #cycles for stop bit - should be 1 1/2 "bit"

	Byte mCRC;

public:


	WavEncoder(int baudrate);

	WavEncoder(TAPFile& tapFile, int baudrate);

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
