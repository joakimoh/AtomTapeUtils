#pragma once

#include <string>
#include <map>
#include <vector>
#include "../shared/TapeProperties.h"
#include "../shared/FileBlock.h"
#include "../shared/Logging.h"


using namespace std;

class ArgParser
{
public:

	TapeProperties tapeTiming;

	double startTime = 0;
	string genDir = "";
	double freqThreshold = 0.25;
	double levelThreshold = 0;
	string wavFile;

	bool cat = false;

	bool genUEF = false;
	bool genCSW = false;
	bool genWAV = false;
	bool genTAP = false;
	bool genSSD = false;

	bool limitBlockNo = false; // If true, the high byte of each tape block no will be discarded

	string dstFileName;

	Logging logging;

	TargetMachine targetMachine = ACORN_ATOM;

	string searchedProgram = "";

private:

	void printUsage(const char *);

	bool mParseSuccess = false;

public:

	ArgParser(int argc, const char* argv[]);

	bool failed();

};

