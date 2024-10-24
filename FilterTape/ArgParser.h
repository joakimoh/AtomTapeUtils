#pragma once

#include <string>
#include <map>
#include <vector>
#include <cmath>
#include "../shared/Logging.h"


using namespace std;

enum FilterType { SINUSOIDAL, SCALE};

class ArgParser
{
public:

	int sinusAmplitude = 16384;
	double saturationLevelLow = 0.8;
	double saturationLevelHigh = 0.8;
	double minPeakDistance = 0.0;
	string outputFileName = "";
	string wavFile;
	int nAveragingSamples = 1;
	double derivativeThreshold = 10;
	bool outputMultipleChannels = false;
	FilterType filterType = SCALE;

	Logging logging;

private:

	void printUsage(const char *);

	bool mParseSuccess = false;

	

public:

	ArgParser(int argc, const char* argv[]);

	bool failed();

};

