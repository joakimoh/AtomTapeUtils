#include "ArgParser.h"
#include <sys/stat.h>
#include <filesystem>
#include <iostream>
#include <string.h>
#include "../shared//Utility.h"

using namespace std;

bool ArgParser::failed()
{
	return !mParseSuccess;
}

void ArgParser::printUsage(const char* name)
{
	cout << "Generate a TAP file ('Wouter Ras' format - see https://www.stairwaytohell.com/atom/wouterras/)\n";
	cout << "from a BASIC source file.\n\n";
	cout << "Usage:\t" << name << " <program file> [-bbm] [-o <output file>] [-v]\n\n";
	cout << "<program file>:\n\tAcorn Atom/BBC Micro BASIC program source file to decode\n\n";
	cout << "If no output file is specified, the output file name will default to the\n";
	cout << "input file name excluding extension.\n\n";
	cout << "-v:\n\tVerbose output\n\n";
	cout << "-bbm:\n\tGenerate for BBC Micro (default is Acorn Atom).\n\n";
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
		cout << "File '" << argv[1] << "' cannot be opened!\n";
		return;
	}
	srcFileName = argv[1];
	dstFileName = Utility::crDefaultOutFileName(srcFileName, ""); // no file extension !!!

	// Now look for remaining options
	int ac = 2;
	while (ac < argc) {
		if (strcmp(argv[ac], "-o") == 0 && ac + 1 < argc) {
			dstFileName = argv[ac + 1];
			ac++;
		}
		else if (strcmp(argv[ac], "-bbm") == 0) {
			targetMachine = BBC_MODEL_B;
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
