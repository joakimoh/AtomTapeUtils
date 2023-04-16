#pragma once

#include <string>
#include <map>
#include <vector>


using namespace std;

class ArgParser
{
public:

	double mStartTime = 0;
	int mBaudRate = 300;
	string mGenDir = "";
	double mDbgStart = 0, mDbgEnd = -1;
	double mFreqThreshold = 0.1;
	double mLevelThreshold = 0;
	string mWavFile;
	double mMinFBLeadTone = 2;
	double mMinOBLeadTone = 2;
	double mMinTrailerTone = 0.0;
	double mMinMicroLeadTone = 0.0;
	bool mErrorCorrection = false;
	bool tracing = false;

private:

	void printUsage(const char *);

	bool mParseSuccess = false;

public:

	ArgParser(int argc, const char* argv[]);

	bool failed();

};

