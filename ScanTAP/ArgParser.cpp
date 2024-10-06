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
	cout << "Usage:\t" << name << " <TAP file> [-g <dir>] [-v]\n";
	cout << "<TAP file>:\n\tTAP file to decode\n\n";
	cout << "If no output file is specified, each output file name will default to the\n";
	cout << "input file name (excluding extension) with the type-specific suffix (e.g.,  '.dat').\n\n";
	cout << "-g dir:\n\tProvide path to directory where generated files shall be put\n\t- default is work directory\n\n";
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
		cout << "MMC file '" << argv[1] << "' cannot be opened!\n";
		return;
	}
	srcFileName = argv[1];
	dstDir = filesystem::current_path().string();

	int ac = 2;

	while (ac < argc) {
		if (strcmp(argv[ac], "-g") == 0 && ac + 1 < argc) {
			filesystem::path dir_path = argv[ac + 1];
			if (!filesystem::is_directory(dir_path))
				cout << "-g without a valid directory\n";
			dstDir = argv[ac + 1];
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
