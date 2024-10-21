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
	cout << "Provides a hex dump of a file's binary content.\n\n";
	cout << "Usage:\t" << name << " <file> [-o <output file]\n";
	cout << "<file>:\n\tfile to inspect\n\n";
	cout << "If no output file is specified, the output file only go to stdout.\n\n";
	cout << "\n";
}

ArgParser::ArgParser(int argc, const char* argv[])
{

	if (argc <= 1) {
		printUsage(argv[0]);
		return;
	}

	srcFileName = argv[1];


	int ac = 2;

	while (ac < argc) {
		if (strcmp(argv[ac], "-o") == 0 && ac + 1 < argc) {

			dstFileName = argv[ac + 1];
			ac++;
		}
		else {
			cout << "Unknown option " << argv[ac] << "\n";
			printUsage(argv[0]);
			return;
		}
		ac++;
	}

	mParseSuccess = true;
}