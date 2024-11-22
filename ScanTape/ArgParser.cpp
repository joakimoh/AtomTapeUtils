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

void ArgParser::printUsage(const char *name)
{
	cout << "Scans a tape file for program files and generates either a set of\n";
	cout << "files of different formats per detected program or a new tape or disc file\n";
	cout << "with the content being the detected (and selected) programs.\n\n";
	cout << "If the audio file is of WAV type and is of poor quality, you should run \n";
	cout << "FilterTape on it first before attempting to scan it for programs...\n\n";
	cout << "Usage:\t" << name << " <WAV/CSW/UEF file> [-v] [-bbm] [-n <program>] [-b <baud rate>]  [-pot]\n";
	cout << "\t-g <dir> | -uef <file> | -wav <file> | -csw <file> | -tap <file> | -ssd <file> | -c\n";
	cout << "\t<advanced options>\n\n";
	cout << "<WAV/CSW/UEF file>:\n\t16-bit PCM WAV/CSW/UEF file to decode.\n\n";
	cout << "-v:\n\tVerbose output.\n\n";
	cout << "-bbm:\n\tScan for BBC Micro (default is Acorn Atom).\n\n";
	cout << "-n <program>:\n\tOnly search for (and extract) <program>.\n\n";
	cout << "-b <baud rate>\n\tBaud rate (default iss 300 without option -bbm selected and 1200 with option -bbm selected).\n\n";
	cout << "-pot:\n\tPreserve original tape timing when generating UEF/CSW/WAV files - default is " << (tapeTiming.preserve?"preserved":"not preserved") << ".\n\n";
	cout << "-g <dir>:\n\tDirectory to put generated files in\n\t- default is work directory.\n\n";
	cout << "-uef <file>:\n\tGenerate one UEF tape file with all successfully decoded programs.\n\n";
	cout << "-csw <file>:\n\tGenerate one CSW tape file with all successfully decoded programs.\n\n";
	cout << "-wav <file>:\n\tGenerate one WAV tape file with all successfully decoded programs.\n\n";
	cout << "-tap <file>:\n\tGenerate one TAP tape file with all successfully decoded programs.\n\n";
	cout << "-ssd <file>:\n\tGenerate one disc image (SSD) file with all successfully decoded programs.\n";
	cout << "\tThe required format for the files (.ssd or .dsd) will be selected by the utility itself\n";
	cout << "\tbased on the no of files (<= 31 => .ssd; >31 && <=62 => .dsd). The original extension (if any)\n";
	cout << "\tof the file will be ignored for that reason.\n\n";
	cout << "-c:\n\tOnly output a catalogue of the files found on the tape.\n\n";

	cout << "\nADVANCED OPTIONS:\n\n";
	cout << "-lbno:\n\tLimit block no. If enabled, the high byte of each tape block no will be discarded.\n";
	cout << "\tUseful for tape blocks with the high byte of the block no being incorrect.\n\n";
	cout << "-s <start time>:\n\tThe time to start detecting files from\n\t- default is 0.\n\n";
	cout << "-f <freq tolerance>:\n\tTolerance of the 1200/2400 frequencies [0,1[\n\t- default is 0.25.\n\n";
	cout << "-l <level tolerance>:\n\tSchmitt-trigger level tolerance [0,1[\n\t- default is 0.\n\n";
	cout << "-lt <d>:\n\tThe duration of the first block's lead tone\n\t- default is " << tapeTiming.nomBlockTiming.firstBlockLeadToneDuration << " s.\n\n";
	cout << "-slt <d>:\n\tThe duration of the subsequent block's lead tone\n\t- default is " << tapeTiming.nomBlockTiming.otherBlockLeadToneDuration << " s.\n\n";
	cout << "-ml <d>:\n\tThe duration of a micro lead tone preceeding a data block\n\t- default is " << tapeTiming.nomBlockTiming.microLeadToneDuration << " s.\n\n";
	cout << "-t:\n\tTurn on tracing showing detected faults.\n\n";
	cout << "-d <debug start time> <debug stop time>:\n\tTape file time range (format hh:mm:ss) for which debugging shall be turned on\n\t- default is off for all times.\n\n";

	

	cout << "\n";
}

ArgParser::ArgParser(int argc, const char* argv[])
{

	genDir = filesystem::current_path().string();

	if (argc <= 1) {
		printUsage(argv[0]);
		return;
	}

	genDir = "";

	filesystem::path fin_path = argv[1];
	if (!filesystem::exists(fin_path)) {
		cout << "WAV file '" << argv[1] << "' cannot be opened!\n";
		return;
	}
	wavFile = argv[1];

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
	bool genFiles = false;
	while (ac < argc) {
		if (strcmp(argv[ac], "-lbno") == 0) {
			limitBlockNo = true;
		}
		else if (strcmp(argv[ac], "-c") == 0) {
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
		else if (strcmp(argv[ac], "-tap") == 0) {
			genTAP = true;
			dstFileName = argv[ac + 1];
			ac++;
		}
		else if (strcmp(argv[ac], "-ssd") == 0) {
			genSSD = true;
			dstFileName = argv[ac + 1];
			ac++;
		}
		else if (strcmp(argv[ac], "-bbm") == 0) {
			// Nothing to do here as already handled above
		}
		else if (strcmp(argv[ac], "-g") == 0 && ac + 1 < argc) {
			filesystem::path dir_path=  argv[ac + 1];
			if (!filesystem::is_directory(dir_path))
				cout << "-g without a valid directory\n";
			genDir = argv[ac+1];
			ac++;
			genFiles = true;
		}
		else if (strcmp(argv[ac], "-pot") == 0) {
			tapeTiming.preserve = true;
		}
		else if (strcmp(argv[ac], "-v") == 0) {
			logging.verbose = true;
		}
		else if (strcmp(argv[ac], "-d") == 0 && ac + 2 < argc) {
			double t1 = Utility::decodeTime(argv[ac + 1]);
			//double t1 = strtod(argv[ac + 1], NULL);
			double t2 = Utility::decodeTime(argv[ac + 2]);
			//double t2 = strtod(argv[ac + 2], NULL);
			if (t1 <= 0 || t2 <= 0 || t2 <= t1)
				cout << "-d without valid non-zero start and stop times\n";
			else {
				logging.tracing = true;
				logging.start = t1;
				logging.end = t2;
				ac += 2;
			}
		}
		else if (strcmp(argv[ac], "-f") == 0 && ac + 1 < argc) {
			double freq_threshold = strtod(argv[ac + 1], NULL);
			if (freq_threshold <= 0 || freq_threshold >= 0.9)
				cout << "-f without a valid non-zero frequency threshold\n";
			else {
				freqThreshold = freq_threshold;
				ac++;
			}
		}
		else if (strcmp(argv[ac], "-l") == 0 && ac + 1 < argc) {
			double level_threshold = strtod(argv[ac + 1], NULL);
			if (level_threshold < 0 || level_threshold >= 0.9)
				cout << "-l without a valid level threshold\n";
			else {
				levelThreshold = strtod(argv[ac + 1], NULL);
				ac++;
			}
		}
		else if (strcmp(argv[ac], "-b") == 0) {
			long baud_rate = strtol(argv[ac + 1], NULL, 10);
			if (baud_rate != 300 && baud_rate != 1200)
				cout << "-b without a valid baud rate\n";
			else {
				tapeTiming.baudRate = stoi(argv[ac + 1]);
				ac++;
			}
		}

		else if (strcmp(argv[ac], "-s") == 0 && ac + 1 < argc) {
			double val = strtod(argv[ac + 1], NULL);
			if (val < 0)
				cout << "-s without a valid start time\n";
			else {
				startTime = val;
				ac++;
			}
		}
		else if (strcmp(argv[ac], "-lt") == 0 && ac + 1 < argc) {
			double val = strtod(argv[ac + 1], NULL);
			if (val < 0)
				cout << "-lt without a valid  tone duration\n";
			else {
				tapeTiming.minBlockTiming.firstBlockLeadToneDuration = val;
				ac++;
			}
		}
		else if (strcmp(argv[ac], "-slt") == 0 && ac + 1 < argc) {
			double val = strtod(argv[ac + 1], NULL);
			if (val < 0)
				cout << "-lt without a valid  tone duration\n";
			else {
				tapeTiming.minBlockTiming.otherBlockLeadToneDuration = val;
				ac++;
			}
		}	
		else if (strcmp(argv[ac], "-ml") == 0 && ac + 1 < argc) {
			double start_time = strtod(argv[ac + 1], NULL);
			if (start_time < 0)
				cout << "-s without a valid tone duration\n";
			else {
				tapeTiming.minBlockTiming.microLeadToneDuration = strtod(argv[ac + 1], NULL);
				ac++;
			}
		}
		else if (strcmp(argv[ac], "-t") == 0) {
			logging.tracing = true;
		}
		else {
			cout << "Unknown option " << argv[ac] << "\n";
			printUsage(argv[0]);
			return;
		}
		ac++;
	}

	if ((int)genFiles + (int)genUEF + (int)genWAV + (int)genCSW + (int)genTAP + (int)genSSD > 1) {
		cout << "Only one of options -g, -uef, -wav, -csw, -tap and -ssd can be specifed!\n";
		printUsage(argv[0]);
		return;
	}

	if ((int)genFiles + (int)genUEF + (int)genWAV + (int)genCSW + (int)genTAP + (int)genSSD == 1 && cat) {
		cout << "Only one of options -g, -uef, -wav, -csw, -tap and -ssd can be specifed!\n";
		printUsage(argv[0]);
		return;
	}

	if (cat && argc < 3) {
		printUsage(argv[0]);
		return;
	}

	mParseSuccess = true;
}
