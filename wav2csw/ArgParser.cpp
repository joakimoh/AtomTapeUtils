#include "ArgParser.h"
#include <sys/stat.h>
#include <filesystem>
#include <iostream>
#include <string.h>
#include "../shared/Utility.h"

using namespace std;

bool ArgParser::failed()
{
	return !mParseSuccess;
}

void ArgParser::printUsage(const char* name)
{
	cout << "Generate a CSW file from a WAV audio file.\n\n";
	cout << "Made in a machine-independent way and without any filtering applied.\n";
	cout << "If the audio file is of poor quality you should run FilterTape on it first\n";
	cout << "before attempting to convert it to CSW format...\n\n";
	cout << "Usage:\t" << name << " <WAV file> [-o <output file] [-v]\n";
	cout << "<WAV file>:\nWAV file to decode\n";
	cout << "\n";
	cout << "If no output file is specified, the output file name will default to the\n";
	cout << "input file name (excluding extension) suffixed with '.csw'.\n";
	cout << "\n";
	cout << "-v:\n\tVerbose output\n\n";
	cout << "\n";
}

ArgParser::ArgParser(int argc, const char* argv[])
{

	if (argc <= 1) {
		printUsage(argv[0]);
		return;
	}

	srcFileName = argv[1];

	dstFileName = Utility::crDefaultOutFileName(srcFileName, "wav");

	int ac = 2;
	while (ac < argc) {
		if (strcmp(argv[ac], "-o") == 0 && ac + 1 < argc) {
			dstFileName = argv[ac + 1];
			ac++;
		}
		else if (strcmp(argv[ac], "-v") == 0) {
			logging.verbose = true;
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
