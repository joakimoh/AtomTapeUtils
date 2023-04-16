#pragma once
#ifndef ARG_PARSER_H
#define ARG_PARSER_H


#include <string>
#include <map>
#include <vector>


using namespace std;

class ArgParser
{
public:


	string mDstFileName;
	string mSrcFileName;
	int mBaudrate = 300;



private:

	void printUsage(const char*);

	bool mParseSuccess = false;

public:

	ArgParser(int argc, const char* argv[]);

	bool failed();

};

#endif