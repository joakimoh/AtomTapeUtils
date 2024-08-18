#pragma once

#include <string>
#include <map>
#include <vector>


using namespace std;

class ArgParser
{
public:


	string dstFileName;
	string srcFileName;
	bool verbose = false;
	bool bbcMicro = false;

private:

	void printUsage(const char*);

	bool mParseSuccess = false;

	

public:

	ArgParser(int argc, const char* argv[]);

	bool failed();

};

