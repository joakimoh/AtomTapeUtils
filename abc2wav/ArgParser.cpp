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
	cout << "Generate a 16-bit PCM WAV audio file based on a program file.\n\n";
	cout << "Usage:\t" << name << " <ABC file> [-o <output file>] [-v] [-bbm] [-b <baud rate>]\n";
	cout << "\t<advanced options>]\n\n";
	cout << "<ABC file>:\n\tAcorn Atom BASIC program file to decode\n";
	cout << "\n";
	cout << "If no output file is specified, the output file name will default to the\n";
	cout << "input file name (excluding extension) suffixed with '.uef'.\n";
	cout << "\n";
	cout << "-v:\n\tVerbose output\n\n";
	cout << "-bbm:\n\tTarget machine is BBC Micro (default is Acorn Atom)\n\n";
	cout << "-b <baud rate>\n\tBaud rate (default iss 300 without option -bbm selected and 1200 with option -bbm selected).\n\n";
	cout << "\nADVANCED OPTIONS:\n\n";
	cout << "-lt <d>:\n\tThe duration of the first block's lead tone\n\t- default is " << tapeTiming.nomBlockTiming.firstBlockLeadToneDuration << " s\n\n";
	cout << "-slt <d>:\n\tThe duration of the subsequent block's lead tone\n\t- default is " << tapeTiming.nomBlockTiming.otherBlockLeadToneDuration << " s\n\n";
	cout << "-ml <d>:\n\tThe duration of a micro lead tone preceeding a data block\n\t- default is " << tapeTiming.nomBlockTiming.microLeadToneDuration << " s\n\n";
	cout << "-fg <d>:\n\tThe duration of the gap before the first block\n\t- default is " << tapeTiming.nomBlockTiming.firstBlockGap << " s\n\n";
	cout << "-sg <d>:\n\tThe duration of the gap before the other blocks\n\t- default is " << tapeTiming.nomBlockTiming.blockGap << " s\n\n";
	cout << "-lg <d>:\n\tThe duration of the gap after the last block\n\t- default is " << tapeTiming.nomBlockTiming.lastBlockGap << " s\n\n";
	cout << "-ps <phase_shift>:\n\tPhase shift when transitioning from high to low tone [0,180] degrees\n\t- default is " << tapeTiming.phaseShift << " degrees\n\n";
	cout << "\n";
}

ArgParser::ArgParser(int argc, const char* argv[])
{

	if (argc <= 1) {
		printUsage(argv[0]);
		return;
	}

	srcFileName = argv[1];

	dstFileName = Utility::crDefaultOutFileName(srcFileName, "wav");

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
		else if (strcmp(argv[ac], "-b") == 0) {
			tapeTiming.baudRate = stoi(argv[ac + 1]);
			if (tapeTiming.baudRate != 300 && tapeTiming.baudRate != 1200)
				cout << "-b without a valid baud rate\n";
			else
				ac++;
		}
		else if (strcmp(argv[ac], "-ps") == 0) {
			tapeTiming.phaseShift = stoi(argv[ac + 1]);
			if (tapeTiming.phaseShift < 0 || tapeTiming.phaseShift > 180)
				cout << "-ps without a valid phase_shift\n";
			else
				ac++;
		}
		else if (strcmp(argv[ac], "-lt") == 0 && ac + 1 < argc) {
			double val = strtod(argv[ac + 1], NULL);
			if (val < 0)
				cout << "-lt without a valid  tone duration\n";
			else {
				tapeTiming.nomBlockTiming.firstBlockLeadToneDuration = val;
				ac++;
			}
		}
		else if (strcmp(argv[ac], "-slt") == 0 && ac + 1 < argc) {
			double val = strtod(argv[ac + 1], NULL);
			if (val < 0)
				cout << "-lt without a valid  tone duration\n";
			else {
				tapeTiming.nomBlockTiming.otherBlockLeadToneDuration = val;
				ac++;
			}
		}
		else if (strcmp(argv[ac], "-ml") == 0 && ac + 1 < argc) {
			double start_time = strtod(argv[ac + 1], NULL);
			if (start_time < 0)
				cout << "-s without a valid tone duration\n";
			else {
				tapeTiming.nomBlockTiming.microLeadToneDuration = strtod(argv[ac + 1], NULL);
				ac++;
			}
		}
		else if (strcmp(argv[ac], "-fg") == 0 && ac + 1 < argc) {
			double val = strtod(argv[ac + 1], NULL);
			if (val < 0)
				cout << "-lt without a valid gap duration\n";
			else {
				tapeTiming.nomBlockTiming.firstBlockGap = val;
				ac++;
			}
		}
		else if (strcmp(argv[ac], "-sg") == 0 && ac + 1 < argc) {
			double val = strtod(argv[ac + 1], NULL);
			if (val < 0)
				cout << "-lt without a valid gap duration\n";
			else {
				tapeTiming.nomBlockTiming.blockGap = val;
				ac++;
			}
		}
		else if (strcmp(argv[ac], "-lg") == 0 && ac + 1 < argc) {
			double val = strtod(argv[ac + 1], NULL);
			if (val < 0)
				cout << "-s without a valid gap duration\n";
			else {
				tapeTiming.nomBlockTiming.lastBlockGap = val;
				ac++;
			}
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
