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
	cout << "Generates a 16-bit PCM WAV audio file from a CSW file.\n\n";
	cout << "Usage:\t" << name << " <CSW file> [-o <output file] [-v] [-p]\n";
	cout << "\n";
	cout << "<CSW file>:\n\tCSW file to decode\n";
	cout << "\n";
	cout << "If no output file is specified, the output file name will default to the\n";
	cout << "input file name (excluding extension) suffixed with '.wav'.\n";
	cout << "\n";
	cout << "-p:\n\tGenerate pulses (default is to generate sinus waves\n\n";
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

	// Now look for remaining options
	int ac = 2;
	while (ac < argc) {
		if (strcmp(argv[ac], "-o") == 0 && ac + 1 < argc) {
			dstFileName = argv[ac + 1];
			ac++;
		}
		else if (strcmp(argv[ac], "-v") == 0) {
			logging.verbose = true;
		}
		else if (strcmp(argv[ac], "-p") == 0) {
			outputPulses = true;
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
