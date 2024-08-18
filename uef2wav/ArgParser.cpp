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
	cout << "Usage:\t" << name << " <UEF file> [-o <output file] [-b <b>] [-lt <d>] [-slt <d>]\n";
	cout <<	"\t[-ml <d>] [-fg <d>] [-sg <d>] [-lg <d>] [-ps <phase_shift>] [-v]\n";
	cout << "\t[-pot] [-f <sample freq>] [-bbm]\n\n";
	cout << "<UEF file>:\n\tUEF file to decode\n";
	cout << "\n";
	cout << "If no output file is specified, the output file name will default to the\n";
	cout << "input file name (excluding extension) suffixed with '.wav'.\n";
	cout << "\n";
	cout << "-lt <d>:\n\tThe duration of the first block's lead tone\n\t- default is " << tapeTiming.nomBlockTiming.firstBlockLeadToneDuration << " s\n\n";
	cout << "-slt <d>:\n\tThe duration of the subsequent block's lead tone\n\t- default is " << tapeTiming.nomBlockTiming.otherBlockLeadToneDuration << " s\n\n";
	cout << "-ml <d>:\n\tThe duration of a micro lead tone preceeding a data block\n\t- default is " << tapeTiming.nomBlockTiming.microLeadToneDuration << " s\n\n";
	cout << "-fg <d>:\n\tThe duration of the gap before the first block\n\t- default is " << tapeTiming.nomBlockTiming.firstBlockGap << " s\n\n";
	cout << "-sg <d>:\n\tThe duration of the gap before the other blocks\n\t- default is " << tapeTiming.nomBlockTiming.blockGap << " s\n\n";
	cout << "-lg <d>:\n\tThe duration of the gap after the last block\n\t- default is " << tapeTiming.nomBlockTiming.lastBlockGap << " s\n\n";
	cout << "-b baudrate:\n\tBaudrate (300 or 1200)\n\t- default is " << tapeTiming.baudRate << "\n\n";
	cout << "-ps <phase_shift>:\n\tPhase shift when transitioning from high to low tone [0,180] degrees\n\t- default is " << tapeTiming.half_cycle << " degrees\n\n";
	cout << "-pot:\n\tPreserve original tape timing when generating the CSW file - default is " << mPreserveOriginalTiming << "\n\n";
	cout << "-f <sample freq>:\n\tSample frequency to use - default is " << mSampleFreq << "\n\n";
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

	dstFileName = crDefaultOutFileName(srcFileName, "wav");

	int ac = 2;
	// First search for option '-bbm' to select target machine and the
	// related default timing properties
	tapeTiming = atomTiming;
	while (ac < argc) {
		if (strcmp(argv[ac], "-bbm") == 0) {
			bbcMicro = true;
			tapeTiming = bbmTiming;
		}
		ac++;
	}

	// Now lock for remaining options
	ac = 2;
	while (ac < argc) {
		if (strcmp(argv[ac], "-bbm") == 0) {
			// Nothing to do here as already handled above
		}
		else if (strcmp(argv[ac], "-o") == 0 && ac + 1 < argc) {
			dstFileName = argv[ac + 1];
			ac++;
		}
		else if (strcmp(argv[ac], "-f") == 0) {
			long freq = strtol(argv[ac + 1], NULL, 10);
			if (freq < 0)
				cout << "-b without a valid baud rate\n";
			else {
				mSampleFreq = stoi(argv[ac + 1]);
				ac++;
			}
		}
		else if (strcmp(argv[ac], "-pot") == 0) {
			mPreserveOriginalTiming = true;
		}
		else if (strcmp(argv[ac], "-v") == 0) {
			verbose = true;
		}
		else if (strcmp(argv[ac], "-b") == 0) {
			tapeTiming.baudRate = stoi(argv[ac + 1]);
			if (tapeTiming.baudRate != 300 && tapeTiming.baudRate != 1200)
				cout << "-b without a valid baud rate\n";
			else
				ac++;
		}
		else if (strcmp(argv[ac], "-ps") == 0) {
			tapeTiming.half_cycle = stoi(argv[ac + 1]);
			if (tapeTiming.half_cycle < 0 || tapeTiming.half_cycle > 180)
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
