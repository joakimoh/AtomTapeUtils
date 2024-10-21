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
	cout << "Generates a CSW file from an UEF file.\n\n"; 
	cout << "Usage:\t" << name << " <UEF file> [-pot] [-f <sample freq>] [-v] [-bbm] [-o <output file]\n";
	cout << "<UEF file>:\n\tUEF file to decode\n\n";
	cout << "-pot:\n\tPreserve original tape timing when generating the CSW file - default is " << mPreserveOriginalTiming << "\n\n";
	cout << "-f <sample freq>:\n\tSample frequency to use - default is " << mSampleFreq << "\n\n";
	cout << "-v:\n\tVerbose output\n\n";
	cout << "If no output file is specified, the output file name will default to the\n";
	cout << "input file name (excluding extension) suffixed with '.csw'.\n\n";
	cout << "-bbm:\nScan for BBC Micro (default is Acorn Atom)\n\n";
	cout << "\n";
}

ArgParser::ArgParser(int argc, const char* argv[])
{

	if (argc <= 1) {
		printUsage(argv[0]);
		return;
	}

	srcFileName = argv[1];
	dstFileName = Utility::crDefaultOutFileName(srcFileName, "csw");


	int ac = 2;
	// First search for option '-bbm' and '-atm' to select target machine and the
	// related default timing properties
	tapeTiming = defaultTiming; // Default for UEF file
	while (ac < argc) {
		if (strcmp(argv[ac], "-bbm") == 0) {
			targetMachine = BBC_MODEL_B;
			tapeTiming = bbmTiming;
		}
		else if (strcmp(argv[ac], "-atm") == 0) {
			targetMachine = ACORN_ATOM;
			tapeTiming = bbmTiming;
		}
		ac++;
	}

	// Now look for remaining options
	ac = 2;
	while (ac < argc) {
		if (strcmp(argv[ac], "-bbm") == 0) {
			// Nothing to do here as already handled above
		}
		else if (strcmp(argv[ac], "-atm") == 0) {
			// Nothing to do here as already handled above
		}
		else if (strcmp(argv[ac], "-o") == 0 && ac + 1 < argc) {

			dstFileName = argv[ac+1];
			ac++;
		}
		else if (strcmp(argv[ac], "-f") == 0) {
			long freq = strtol(argv[ac + 1], NULL, 10);
			if (freq < 0)
				cout << "-b without a valid baud rate\n";
			else {
				mSampleFreq = stoi(argv[ac + 1]);
				ac++;
			}
		}
		else if (strcmp(argv[ac], "-v") == 0) {
			logging.verbose = true;
		}
		else if (strcmp(argv[ac], "-pot") == 0) {
			mPreserveOriginalTiming = true;
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
