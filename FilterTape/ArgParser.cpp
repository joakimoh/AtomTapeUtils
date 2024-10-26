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
	
	cout << "Filters a tape audio file (of poor quality) so that the result is a tape file without a DC offset and\n";
	cout << "with normalised amplitudes. The normalisation could be made either by scaling of the original samples\n";
	cout << "(default) or by reshaping (option -sine) the original samples as ideal sinusoidal curves.\n\n";
	cout << "The sensitivity is by default high so any change in amplitude would be detected as a transition\n";
	cout << "(from low to high or high to low). Tape segments that are silent (but slightly noisy) could potentially\n";
	cout << "therefore be interpreted as transitions with phantom sinusoidal curves (of random frequency) being created.\n";
	cout << "For some tape decoders this could be a problem but if it is a problem the sensitivity can be lowered using\n";
	cout << "the option - d.\n";
	cout << "Toggling between the reshaping and scaling could also be done if the filtering doesn't yield the expected result.\n";
	cout << "There are many other settings but the -d and -scale options are the ones that\n";
	cout << "usually could need a bit of variation to get it 'right'.\n\n";
	cout << "Usage:\t" << name << " <WAV file> [-o <output file] [-sine] [-extremums]\n";
	cout << "\t [-a <#samples>] [-d <threshold>] [-p <distance>] [-m]\n"; 
	cout << "\t [-sl <saturation level>] [-sh <saturation high>] [-v]\n";
	cout << "\n";
	cout << "If no output file is specified, the output file name will default to the\n";
	cout << "input file name (excluding extension) suffixed with '_out.wav'.\n\n";
	cout << "-v:\n\tVerbose output\n\n";
	cout << "-sine:\n\tThe original samples will be replaced by ideal sinusoidal curves based on the same detected extremes.\n";
	cout << "\n";
	cout << "-extremums:\n\tOnly generate the extremums (min, max) detected for the tape. Useful for debugging.\n";
	cout << "\n";
	cout << "-a <n>:\n\tIf non-zero this specifies the number of samples 2n + 1 around a sample point that are \n";
	cout << "\taveraged together.\n";
	cout << "\tDefault: 1\n";
	cout << "\n";
	cout << "-d <threshold>:\n\tThe upper boundary [0, 1000] for considering a derivative to be zero.\n"; 
	cout << "\tDefault: 10\n";
	cout << "\n";
	cout << "-p <distance>:\n\tThe min distance between peaks [0.0, 0.5] as percentage of 2400 tone length.\n";
	cout << "\tDefault: 0.0\n";
	cout << "\n";
	cout << "-sl <saturation level>:\n\tThe low saturation level [0.25, 1] (relatively zero). Under this threshold the signal will be cut.\n";
	cout << "\tDefault: 0.8\n";
	cout << "\n";
	cout << "-sh <saturation level>:\n\tThe high saturation level [0.25, 1] (relatively zero). Over this threshold the signal will be cut.\n";
	cout << "\tDefault: 0.8\n";
	cout << "\n";
	cout << "-m:\n\tWill create a WAW file that includes both the original samples and the filtered ones.\n\n";
	cout << "\n";
}

ArgParser::ArgParser(int argc, const char* argv[])
{


	if (argc <= 1) {
		printUsage(argv[0]);
		return;
	}

	// Get input file name
	wavFile = argv[1];	

	// Create default output file name based on input file name
	outputFileName = Utility::crDefaultOutFileName(wavFile);


	// Check for options
	int ac = 2;
	while (ac < argc) {
		if (strcmp(argv[ac], "-o") == 0 && ac + 1 < argc) {
			outputFileName =  argv[ac + 1];
			ac++;
		}
		else if (strcmp(argv[ac], "-v") == 0) {
			logging.verbose = true;
		}
		else if (strcmp(argv[ac], "-sine") == 0) {
			filterType = SINUSOIDAL;
		}
		else if (strcmp(argv[ac], "-extremums") == 0) {
			filterType = EXTREMUM;
		}
		else if (strcmp(argv[ac], "-p") == 0 && ac + 1 < argc) {
			minPeakDistance = strtod(argv[ac + 1], NULL);
			if (minPeakDistance < 0 || minPeakDistance > 0.5) {
				cout << "-p without a valid threshold\n";
				printUsage(argv[0]);
				return;
			}
			else
				ac++;
		}
		else if (strcmp(argv[ac], "-sh") == 0 && ac + 1 < argc) {
			saturationLevelHigh = strtod(argv[ac + 1], NULL);
			if (saturationLevelHigh < 0.25 || saturationLevelHigh > 1) {
				cout << "-p without a valid threshold\n";
				printUsage(argv[0]);
				return;
			}
			else
				ac++;
		}
		else if (strcmp(argv[ac], "-sl") == 0 && ac + 1 < argc) {
			saturationLevelLow = strtod(argv[ac + 1], NULL);
			if (saturationLevelLow < 0.25 || saturationLevelLow > 1) {
				cout << "-p without a valid threshold\n";
				printUsage(argv[0]);
				return;
			}
			else
				ac++;
		}
		else if (strcmp(argv[ac], "-m") == 0) {
			outputMultipleChannels = true;
		}
		else if (strcmp(argv[ac], "-a") == 0) {
			nAveragingSamples = stoi(argv[ac + 1]);
			if (nAveragingSamples < 0) {
				cout << "-a without a valid no of samples\n";
				printUsage(argv[0]);
				return;
			}
			else
				ac++;
		}
		else if (strcmp(argv[ac], "-d") == 0) {
			derivativeThreshold = strtod(argv[ac + 1], NULL);
			if (derivativeThreshold < 0 || derivativeThreshold > 1000) {
				cout << "-d without a valid threshold\n";
				printUsage(argv[0]);
				return;
			}
			else
				ac++;
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
