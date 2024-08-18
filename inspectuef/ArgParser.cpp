#include "ArgParser.h"
#include <sys/stat.h>
#include <filesystem>
#include <iostream>
#include "../shared/Utility.h"
#include <string.h>

using namespace std;

bool ArgParser::failed()
{
	return !mParseSuccess;
}

void ArgParser::printUsage(const char* name)
{
	cout << "Usage:\t" << name << " <file>\n";
	cout << "<file>:\n\tfile to inspect\n\n";
	cout << "\n";
}

ArgParser::ArgParser(int argc, const char* argv[])
{

	if (argc != 2) {
		printUsage(argv[0]);
		return;
	}

	srcFileName = argv[1];

	mParseSuccess = true;
}