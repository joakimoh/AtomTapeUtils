#pragma once

#include <string>
#include <map>
#include <vector>
#include "../shared/Logging.h"


using namespace std;

class ArgParser
{
public:


	string dstFileName;
	string srcFileName;
	Logging logging;

private:

	void printUsage(const char*);

	bool mParseSuccess = false;

	

public:

	ArgParser(int argc, const char* argv[]);

	bool failed();

};

