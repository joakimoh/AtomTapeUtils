#pragma once

#include <string>
#include <map>
#include <vector>
#include "../shared/Logging.h"
#include "../shared/TapeProperties.h"
#include "../shared/FileBlock.h"



using namespace std;

class ArgParser
{
public:

	TapeProperties tapeTiming;

	string dstDir;
	string srcFileName;
	Logging logging;

	bool cat = false;

	bool genUEF = false;
	bool genCSW = false;
	bool genWAV = false;

	string dstFileName;

	string find_file_name = "";

	TargetMachine targetMachine = ACORN_ATOM;

private:

	void printUsage(const char*);

	bool mParseSuccess = false;



public:

	ArgParser(int argc, const char* argv[]);

	bool failed();

};

