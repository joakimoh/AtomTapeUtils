#pragma once

#include <string>
#include <map>
#include <vector>
#include "FileBlock.h"


using namespace std;

class ArgParser
{
public:


	string dstFileName;
	string srcFileName;
	bool verbose = false;
	TargetMachine targetMachine = ACORN_ATOM;


private:

	void printUsage(const char*);

	bool mParseSuccess = false;

public:

	ArgParser(int argc, const char* argv[]);

	bool failed();

};

