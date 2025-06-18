#pragma once

#include <string>
#include <map>
#include <vector>
#include "../shared/Logging.h"


using namespace std;

class ArgParser
{
public:


	string dirName;
	string fileName;

	Logging logging;

	bool decode = true;

private:

	void printUsage(const char*);

	bool mParseSuccess = false;



public:

	ArgParser(int argc, const char* argv[]);

	bool failed();

};

