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
	cout << "Generate program source code from a binary (BIN) file.\n\n";
	cout << "Usage:\t" << name << " <BIN file> [-o <output file>] [-v] [-bbm]\n";
	cout << "<BIN file>:\n\tBinary file containing a binary (native) Atom/BBC Micro BASIC program\n\n";
	cout << "If no output file is specified, the output file name will default to the\n";
	cout << "input file name (excluding extension) suffixed with '.abc'/'.bbc'.\n\n";
	cout << "-v:\n\tVerbose output\n\n";
	cout << "-bbm:\n\tTarget machine is BBC Micro (default is Acorn Atom)\n\n";
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
		cout << "MMC file '" << argv[1] << "' cannot be opened!\n";
		return;
	}
	srcFileName = argv[1];
	dstFileName = Utility::crDefaultOutFileName(srcFileName, "abc");

	int ac = 2;
	// First search for option '-bbm' to select target machine and the
	// related properties

	while (ac < argc) {
		if (strcmp(argv[ac], "-bbm") == 0) {
			targetMachine = BBC_MODEL_B;
		}
		ac++;
	}

	if (targetMachine)
		dstFileName = Utility::crDefaultOutFileName(srcFileName, "bbc");

	// Now look for remaining options
	ac = 2;
	while (ac < argc) {
		if (strcmp(argv[ac], "-bbm") == 0) {
			// Nothing to do here as already handled above
		}
		else if (strcmp(argv[ac], "-o") == 0 && ac + 1 < argc) {
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
