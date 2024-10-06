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


	string dstFileName;
	string srcFileName;
	TapeProperties tapeTiming;
	Logging logging;
	TargetMachine targetMachine = ACORN_ATOM;

private:

	void printUsage(const char*);

	bool mParseSuccess = false;



public:

	ArgParser(int argc, const char* argv[]);

	bool failed();

};

