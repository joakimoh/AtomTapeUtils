#pragma once

#include <string>
#include <map>
#include <vector>
#include "../shared/TapeProperties.h"
#include "../shared/BlockTypes.h"


using namespace std;

class ArgParser
{
public:

	TapeProperties tapeTiming;

	double startTime = 0;
	string genDir = "";
	double dbgStart = 0, dbgEnd = -1;
	double freqThreshold = 0.1;
	double levelThreshold = 0;
	string wavFile;

	bool mErrorCorrection = false;
	bool tracing = false;

	bool verbose = false;

	TargetMachine targetMachine = ACORN_ATOM;

private:

	void printUsage(const char *);

	bool mParseSuccess = false;

public:

	ArgParser(int argc, const char* argv[]);

	bool failed();

};

