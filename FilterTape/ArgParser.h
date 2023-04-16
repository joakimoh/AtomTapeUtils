#pragma once

#include <string>
#include <map>
#include <vector>


using namespace std;

class ArgParser
{
public:

	double mSinusAmplitude = 16384;
	double mSaturationLevelLow = 0.8;
	double mSaturationLevelHigh = 0.8;
	double mMinPeakDistance = 0.0;
	int mBaudRate = 300;
	string mOutputFileName = "";
	string mWavFile;
	int mNAveragingSamples = 1;
	double mDerivativeThreshold = 10;
	bool mOutputMultipleChannels = false;
	const int MAX_AVERAGING_SAMPLES = (int) round(44100 / 2400 / 8);// No of samples corresponding to 1/8 period of 2400 Hz


private:

	void printUsage(const char *);

	bool mParseSuccess = false;

public:

	ArgParser(int argc, const char* argv[]);

	bool failed();

};

