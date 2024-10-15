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
	cout << "Usage:\t" << name << " <Disc file> [-v] [-bbm]\n";
	cout << " \t -g <generate dir path | -uef <file> | -wav <file> | -csw <file>\n";
	cout << "<Disc file>:\n\tAcorn DFS disc file to decode\n\n";
	cout << "If no output file is specified, each output file name will default to the\n";
	cout << "input file name (excluding extension) with the type-specific suffix (e.g.,  '.dat').\n\n";
	cout << "-g dir:\n\tProvide path to directory where generated files shall be put\n\t- default is work directory\n\n";
	cout << "-v:\n\tVerbose output\n\n";
	cout << "-euf <file>:\nGenerate one UEF tape file with all successfully decoded programs - mutually exclusive w.r.t option '-g'\n\n";
	cout << "-csw <file>:\nGenerate one CSW tape file with all successfully decoded programs - mutually exclusive w.r.t option '-g'\n\n";
	cout << "-wav <file>:\nGenerate one WAV tape file with all successfully decoded programs - mutually exclusive w.r.t option '-g'\n\n";
	cout << "-bbm:\nScan for BBC Micro (default is Acorn Atom)\n\n"; 
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

	ac = 2;
	bool genFiles = false;
	while (ac < argc) {
		if (strcmp(argv[ac], "-bbm") == 0) {
			// Nothing to do here as already covered above
		}
		else if (strcmp(argv[ac], "-c") == 0) {
			cat = true;
		}
		else if (strcmp(argv[ac], "-n") == 0) {
			find_file_name = argv[ac + 1];
			ac++;
		}
		else if (strcmp(argv[ac], "-uef") == 0) {
			genUEF = true;
			dstFileName = argv[ac + 1];
			ac++;
		}
		else if (strcmp(argv[ac], "-csw") == 0) {
			genCSW = true;
			dstFileName = argv[ac + 1];
			ac++;
		}
		else if (strcmp(argv[ac], "-wav") == 0) {
			genWAV = true;
			dstFileName = argv[ac + 1];
			ac++;
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

	if ((int)genFiles + (int)genUEF + (int)genWAV + (int)genCSW > 1) {
		cout << "Only one of options -g, -uef, -csw and -wav can be specifed!\n";
		printUsage(argv[0]);
		return;
	}
	if (cat && argc < 3) {
		printUsage(argv[0]);
		return;
	}


	mParseSuccess = true;
}
