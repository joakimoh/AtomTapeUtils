#include "ArgParser.h"
#include <sys/stat.h>
#include <filesystem>
#include <iostream>
#include "../shared//Utility.h"
#include <string.h>

using namespace std;

bool ArgParser::failed()
{
	return !mParseSuccess;
}

void ArgParser::printUsage(const char* name)
{
	cout << "Usage:\t" << name << " <DATA file> [-o <output file>] [-v]\n";
	cout << "<DAT file>:\n\tDATA file to convert to binary file\n\n";
	cout << "If no output file is specified, the output file name will default to the\n";
	cout << "input file name (excluding extension) suffixed with '.bin'.\n\n";
	cout << "-v:\n\tVerbose output\n\n";
	cout << "\n";
}

ArgParser::ArgParser(int argc, const char* argv[])
{



	if (argc <= 1) {
		printUsage(argv[0]);
		return;
	}

	filesystem::path fin_path = argv[1];
	if (!filesystem::exists(fin_path)) {
		cout << "ABC file '" << argv[1] << "' cannot be opened!\n";
		return;
	}
	srcFileName = argv[1];
	dstFileName = crDefaultOutFileName(srcFileName, "bin");

	int ac = 2;

	while (ac < argc) {
		if (strcmp(argv[ac], "-o") == 0 && ac + 1 < argc) {
			dstFileName = argv[ac + 1];
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
