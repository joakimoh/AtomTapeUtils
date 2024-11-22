#pragma once
#ifndef ARG_PARSER_H
#define ARG_PARSER_H


#include <string>
#include <map>
#include <vector>
#include "../shared/TapeProperties.h"
#include "../shared/Logging.h"


using namespace std;

class ArgParser
{
public:


	string dstFileName;
	string srcFileName;
	TapeProperties tapeTiming;
	Logging logging;
	bool outputPulses = false;




private:

	void printUsage(const char*);

	bool mParseSuccess = false;

public:

	ArgParser(int argc, const char* argv[]);

	bool failed();

};

#endif