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

void ArgParser::printUsage(const char *name)
{
	cout << "Generate UEF, DAT, BIN and TAP files based on a program file.\n\n"; 
	cout << "Usage:\t" << name << " <program source file> [-g <output directory>] [-v] [-bbm]\n";
	cout << " \t -g <dir>\n\n";
	cout << "<program source file>:\n\tBASIC program file to decode\n\n";
	cout << "-v:\n\tVerbose output\n\n";
	cout << "-g <dir>:\n\tDirectory to put generated files in\n\t- default is work directory.\n\n";
	cout << "-bbm:\n\tTarget machine is BBC Micro (default is Acorn Atom)\n\n";
	cout << "If no output file is specified, the output directory will default to the\n";
	cout << "working directory.\n\n";
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
	dstDir = filesystem::current_path().string();

	int ac = 2;

	while (ac < argc) {
		if (strcmp(argv[ac], "-bbm") == 0) {
			targetMachine = BBC_MODEL_B;
		}
		else if (strcmp(argv[ac], "-g") == 0 && ac + 1 < argc) {
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
