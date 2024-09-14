#pragma once
#ifndef ARG_PARSER_H
#define ARG_PARSER_H


#include <string>
#include <map>
#include <vector>
#include "../shared/TapeProperties.h"
#include "FileBlock.h"


using namespace std;

class ArgParser
{
public:


	string dstFileName;
	string srcFileName;
	TapeProperties tapeTiming;
	bool mPreserveOriginalTiming = false;
	int mSampleFreq = 44100;
	bool verbose = false;
	TargetMachine targetMachine = UNKNOWN_TARGET;



private:

	void printUsage(const char*);

	bool mParseSuccess = false;

public:

	ArgParser(int argc, const char* argv[]);

	bool failed();

};

#endif