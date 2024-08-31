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

void ArgParser::printUsage(const char *name)
{
	cout << "Usage:\t" << name << " <UEF file> [-pot] [-o <output file] [-v] [-bbm]\n";
	cout << "<UEF file>:\n\tUEF file to decode\n\n";
	cout << "If no output file is specified, the output file name will default to the\n";
	cout << "input file name (excluding extension) suffixed with '.dat'.\n\n";
	cout << "-v:\n\tVerbose output\n\n";
	cout << "-bbm:\n\Scan for BBC Micro (default is Acorn Atom)\n\n";
	cout << "\n";
}

ArgParser::ArgParser(int argc, const char* argv[])
{

	if (argc <= 1) {
		printUsage(argv[0]);
		return;
	}

	srcFileName = argv[1];
	dstFileName = Utility::crDefaultOutFileName(srcFileName, "dat");


	int ac = 2;
	// First search for option '-bbm' to select target machine and the
	// related default timing properties
	tapeTiming = atomTiming;
	while (ac < argc) {
		if (strcmp(argv[ac], "-bbm") == 0) {
			targetMachine = BBC_MODEL_B;
			tapeTiming = bbmTiming;
		}
		ac++;
	}

	// Now lock for remaining options
	ac = 2;
	while (ac < argc) {
		if (strcmp(argv[ac], "-bbm") == 0) {
			// Nothing to do here as already handled above
		}
		else if (strcmp(argv[ac], "-o") == 0 && ac + 1 < argc) {

			dstFileName = argv[ac+1];
			ac++;
		}
		else if (strcmp(argv[ac], "-v") == 0) {
			verbose = true;
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
