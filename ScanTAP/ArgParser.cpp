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
	cout << "Scans an Acorn Atom TAP file for program files and generates either a set of\n";
	cout << "files of different formats per detected program or a new tape or disc file\n";
	cout << "with the content being the detected (and selected) programs.\n\n"; cout << "Usage:\t" << name << " <TAP file> [-v] [-n <program>]\n";
	cout << " \t -g <dir> | -uef <file> | -wav <file> | -csw <file> | -ssd <file> | -c\n\n";
	cout << "<TAP file>:\n\tTAP file to decode.\n\n";
	cout << "-v:\n\tVerbose output\n\n";
	cout << "-n <program>:\n\tOnly search for (and extract) <program>.\n\n";
	cout << "-g <dir>:\n\tDirectory to put generated files in\n\t- default is work directory.\n\n";
	cout << "-uef <file>:\n\tGenerate one UEF tape file with all successfully decoded programs.\n\n";
	cout << "-csw <file>:\n\tGenerate one CSW tape file with all successfully decoded programs.\n\n";
	cout << "-wav <file>:\n\tGenerate one WAV tape file with all successfully decoded programs.\n\n";
	cout << "-ssd <file>:\n\tGenerate one disc image (SSD) file with all successfully decoded programs.\n";
	cout << "\tThe required format for the files (.ssd or .dsd) will be selected by the utility itself.\n";
	cout << "\tbased on the no of files (<= 31 => .ssd; >31 && <=62 => .dsd). The original extension (if any)\n";
	cout << "\tof the file will be ignored for that reason.\n\n";
	cout << "-c:\n\tOnly output a catalogue of the files found on the tape.\n\n";
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
		if (strcmp(argv[ac], "-c") == 0) {
			cat = true;
		}
		else if (strcmp(argv[ac], "-n") == 0) {
			searchedProgram = argv[ac + 1];
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
		else if (strcmp(argv[ac], "-ssd") == 0) {
			genSSD = true;
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

	if ((int)genFiles + (int)genUEF + (int)genWAV + (int)genCSW + (int)genSSD > 1) {
		cout << "Only one of options -g, -uef, -wav, -csw and -ssd can be specifed!\n";
		printUsage(argv[0]);
		return;
	}

	if ((int)genFiles + (int)genUEF + (int)genWAV + (int)genCSW + (int)genSSD == 1 && cat) {
		cout << "Only one of options -g, -uef, -wav, -csw and -ssd can be specifed!\n";
		printUsage(argv[0]);
		return;
	}

	if (cat && argc < 3) {
		printUsage(argv[0]);
		return;
	}


	mParseSuccess = true;
}
