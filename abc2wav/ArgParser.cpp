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
	cout << "Usage:\t" << name << " <ABC file> [-o <output file] [-b <baudrate>]\n";
	cout << "<ABC file>:\n\tAcorn Atom BASIC program file to decode\n\n";
	cout << "If no output file is specified, the output file name will default to the\n";
	cout << "input file name (excluding extension) suffixed with '.wav'.\n\n";
	cout << "-b baudrate          :\n\tBaudrate (300 or 1200)\n";
	cout << "\tDefault: 300\n\n";
	cout << "\n";
}

ArgParser::ArgParser(int argc, const char* argv[])
{

	if (argc <= 1) {
		printUsage(argv[0]);
		return;
	}

	mSrcFileName = argv[1];

	mDstFileName = crDefaultOutFileName(mSrcFileName, "wav");

	int ac = 2;

	while (ac < argc) {
		if (strcmp(argv[ac], "-o") == 0 && ac + 1 < argc) {
			mDstFileName = argv[ac + 1];
			ac++;
		}
		else if (strcmp(argv[ac], "-b") == 0) {
			mBaudrate = stoi(argv[ac + 1]);
			if (mBaudrate != 300 && mBaudrate != 1200)
				cout << "-b without a valid baud rate\n";
			else
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
