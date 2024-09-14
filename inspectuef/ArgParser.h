#pragma once

#include <string>
#include <map>
#include <vector>


using namespace std;

class ArgParser
{
public:

	string srcFileName = "";
	string dstFileName = "";


private:

	void printUsage(const char*);

	bool mParseSuccess = false;

public:

	ArgParser(int argc, const char* argv[]);

	bool failed();

};

