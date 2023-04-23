#pragma once

#include <string>
#include <map>
#include <vector>
#include "../shared/TapeProperties.h"


using namespace std;

class ArgParser
{
public:

	TapeProperties tapeTiming;

	double mStartTime = 0;
	string mGenDir = "";
	double mDbgStart = 0, mDbgEnd = -1;
	double mFreqThreshold = 0.1;
	double mLevelThreshold = 0;
	string mWavFile;

	bool mErrorCorrection = false;
	bool tracing = false;

private:

	void printUsage(const char *);

	bool mParseSuccess = false;

public:

	ArgParser(int argc, const char* argv[]);

	bool failed();

};

