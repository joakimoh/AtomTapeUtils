#pragma once
#ifndef WAV_ENCODER_H
#define WAV_ENCODER_H



#include <vector>
#include <string>
#include "TAPCodec.h"
#include "WaveSampleTypes.h"
#include "TapeProperties.h"
#include "BitTiming.h"
#include "UEFCodec.h"


using namespace std;

class WavEncoder
{

private:

	Samples mSamples;
	TapeProperties mTapeTiming;
	bool mUseOriginalTiming = false;

	int mMaxSampleAmplitude = 16384;

	BitTiming mBitTiming;

	Word mCRC;

	int mPhase = 180;

	bool mVerbose = false;

	TargetMachine mTargetMachine = ACORN_ATOM;

	DataEncoding mDefaultEncoding;

public:


	WavEncoder(int sampleFreq, TapeProperties tapeTiming, bool verbose, TargetMachine targetMachine);

	WavEncoder(bool useOriginalTiming, int sampleFreq, TapeProperties tapeTiming, bool verbose, TargetMachine targetMachine);

	bool writeByte(Byte byte, DataEncoding encoding);
	bool writeDataBit(int bit);
	bool writeStartBit();
	bool writeStopBit(DataEncoding encoding);
	bool writeCycle(bool high, unsigned n);
	bool writeTone(double duration);
	bool writeGap(double duration);

	bool writeSamples(string &filePath);


	/*
	 * Encode TAP File structure as WAV file
	 */
	bool encode(TapeFile& tapeFile, string& filePath);
	bool encodeAtom(TapeFile& tapeFile, string& filePath);
	bool encodeBBM(TapeFile& tapeFile, string& filePath);

	bool setBaseFreq(double baseFreq);
	bool setBaudRate(int baudrate);
	bool setPhase(int phase);
};

#endif // ! WAV_ENCODER_H
