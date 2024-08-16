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
	cout << "Usage:\t" << name << " <UEF file> [-o <output file] [-v]\n";
	cout << "<UEF file>:\n\tUEF file to decode\n\n";
	cout << "If no output file is specified, the output file name will default to the\n";
	cout << "input file name (excluding extension) suffixed with '.abc'.\n\n";
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
	


	int ac = 2;

	while (ac < argc) {
		if (strcmp(argv[ac], "-o") == 0 && ac + 1 < argc) {

			dstFileName = argv[ac + 1];
			ac++;
		}
		else if (strcmp(argv[ac], "-bbm") == 0) {
			bbcMicro = true;
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

	if (dstFileName == "") {
		if (bbcMicro)
			dstFileName = crDefaultOutFileName(srcFileName, "bbc");
		else
			dstFileName = crDefaultOutFileName(srcFileName, "abc");
	}

	mParseSuccess = true;
}
