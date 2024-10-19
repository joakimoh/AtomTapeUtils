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

	Logging mDebugInfo;

	TargetMachine mTargetMachine = ACORN_ATOM;

	DataEncoding mDefaultEncoding;

	string mTapeFilePath = "";

	bool encodeAtom(TapeFile& tapeFile);
	bool encodeBBM(TapeFile& tapeFile);

public:


	WavEncoder(int sampleFreq, TapeProperties tapeTiming, Logging logging, TargetMachine targetMachine);

	WavEncoder(bool useOriginalTiming, int sampleFreq, TapeProperties tapeTiming, Logging logging, TargetMachine targetMachine);

	bool writeByte(Byte byte, DataEncoding encoding);
	bool writeDataBit(int bit);
	bool writeStartBit();
	bool writeStopBit(DataEncoding encoding);
	bool writeCycle(bool high, unsigned n);
	static bool writeHalfCycle(Samples& samples, Level &halfCycleLevel, int nSamples);
	static bool writePulse(Samples& samples, Level &halfCycleLevel, int nSamples);
	bool writeTone(double duration);
	bool writeGap(double duration);

	bool writeSamples(string &filePath);


	/*
	 * Encode TAP File structure as WAV file
	 */
	bool openTapeFile(string& filePath);
	bool closeTapeFile();
	bool encode(TapeFile& tapeFile, string& filePath);
	bool encode(TapeFile& tapeFile);


	bool setBaseFreq(double baseFreq);
	bool setBaudRate(int baudrate);
	bool setPhase(int phase);
};

#endif // ! WAV_ENCODER_H
