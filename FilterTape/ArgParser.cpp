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
	cout << "Usage:\t" << name << " <WAV file> [-o <output file] [-b <baud rate>]\n";
	cout << "\t [-a <#samples>] [-d <threshold>] [-p <distance>] [-m]\n"; 
	cout << "\t [-sl <saturation level>] [-sh <saturation high>]\n";
	cout << "\n";
	cout << "-b <baud rate>:\tBaudrate (300 or 1200)\n";
	cout << "\n";
	cout << "-a <n>        :\tIf non-zero (and in the range [1" << ", " << MAX_AVERAGING_SAMPLES << "]) this specifies \n";
	cout << "\t\tthe number of samples 2n + 1 around a sample point that are \n"; 
	cout << "\t\taveraged together.\n";
	cout << "\t\tDefault: 1\n";
	cout << "\n";
	cout << "-d <threshold>:\tThe upper boundary [0, 1000] for considering a derivative to be zero.\n"; 
	cout << "\t\tDefault: 10\n";
	cout << "\n";
	cout << "-p <distance>:\tThe min distance between peaks [0.0, 0.5] as percentage of 2400 tone length.\n";
	cout << "\t\tDefault: 0.0\n";
	cout << "\n";
	cout << "-sl <saturation level>:\tThe low saturation level [0.25, 1] (relatively zero). Under this threshold the signal will be cut.\n";
	cout << "\t\tDefault: 0.8\n";
	cout << "\n";
	cout << "-sh <saturation level>:\tThe high saturation level [0.25, 1] (relatively zero). Over this threshold the signal will be cut.\n";
	cout << "\t\tDefault: 0.8\n";
	cout << "\n";
	cout << "-m            :\tWill create a WAW file that includes both the original samples and the filtered ones.\n\n";
	cout << "If no output file is specified, the output file name will default to the\n";
	cout << "input file name (excluding extension) suffixed with '_out.wav'.\n\n";
	cout << "\n";
}

ArgParser::ArgParser(int argc, const char* argv[])
{


	if (argc <= 1) {
		printUsage(argv[0]);
		return;
	}

	// Get input file name
	mWavFile = argv[1];	

	// Create default output file name based on input file name
	mOutputFileName = crDefaultOutFileName(mWavFile);


	// Check for options
	int ac = 2;
	while (ac < argc) {
		if (strcmp(argv[ac], "-o") == 0 && ac + 1 < argc) {
			mOutputFileName =  argv[ac + 1];
			ac++;
		}
		else if (strcmp(argv[ac], "-p") == 0 && ac + 1 < argc) {
			mMinPeakDistance = strtod(argv[ac + 1], NULL);
			if (mMinPeakDistance < 0 || mMinPeakDistance > 0.5) {
				cout << "-p without a valid threshold\n";
				printUsage(argv[0]);
				return;
			}
			else
				ac++;
		}
		else if (strcmp(argv[ac], "-sh") == 0 && ac + 1 < argc) {
			mSaturationLevelHigh = strtod(argv[ac + 1], NULL);
			if (mSaturationLevelHigh < 0.25 || mSaturationLevelHigh > 1) {
				cout << "-p without a valid threshold\n";
				printUsage(argv[0]);
				return;
			}
			else
				ac++;
		}
		else if (strcmp(argv[ac], "-sl") == 0 && ac + 1 < argc) {
			mSaturationLevelLow = strtod(argv[ac + 1], NULL);
			if (mSaturationLevelLow < 0.25 || mSaturationLevelLow > 1) {
				cout << "-p without a valid threshold\n";
				printUsage(argv[0]);
				return;
			}
			else
				ac++;
		}
		else if (strcmp(argv[ac], "-m") == 0) {
			mOutputMultipleChannels = true;
		}
		else if (strcmp(argv[ac], "-b") == 0) {
			mBaudRate = stoi(argv[ac + 1]);
			if (mBaudRate != 300 && mBaudRate != 1200) {
				cout << "-b without a valid baud rate\n";
				printUsage(argv[0]);
				return;
			}
			else
				ac++;
		}
		else if (strcmp(argv[ac], "-a") == 0) {
			mNAveragingSamples = stoi(argv[ac + 1]);
			if (mNAveragingSamples < 0 || mNAveragingSamples > MAX_AVERAGING_SAMPLES) {
				cout << "-a without a valid no of samples\n";
				printUsage(argv[0]);
				return;
			}
			else
				ac++;
		}
		else if (strcmp(argv[ac], "-d") == 0) {
			mDerivativeThreshold = strtod(argv[ac + 1], NULL);
			if (mDerivativeThreshold < 0 || mDerivativeThreshold > 1000) {
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
