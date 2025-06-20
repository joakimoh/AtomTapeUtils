#include "ArgParser.h"
#include <sys/stat.h>
#include <filesystem>
#include <iostream>
#include <string.h>
#include "../shared//Utility.h"
#include "../shared/Logging.h"

using namespace std;

bool ArgParser::failed()
{
	return !mParseSuccess;
}

void ArgParser::printUsage(const char* name)
{
	cout << "Encodes and decodes MMB files.\n\n";
	cout << "Usage:\t" << name << " [-decode] <MMB src file> [-c] [-g <dst dir>] [-v]\n";
	cout << "\t" << name << " -encode <src_dir> -o <MMB output file>\n";
	cout << "<MMB src file>:\n\MMB file to decode\n\n";
	cout << "<MMB dst file>:\n\MMB file to encode (i.e., generate)\n\n";
	cout << "If no directory is specified, it will default to the\n";
	cout << "current working directory'.\n\n";
	cout << "-v:\n\tVerbose output\n\n";
	cout << "-c:\n\t(decoding only) only output a catalogue\n\n";
	cout << "\n";
}

ArgParser::ArgParser(int argc, const char* argv[])
{

	if (argc <= 2) {
		printUsage(argv[0]);
		return;
	}

	fileName = "";
	dirName = filesystem::current_path().string();

	int ac = 1;
	decode = true;
	int files_provided = 0;
	int n_args = 0;
	while (ac < argc) {
		if (files_provided == 0 && strcmp(argv[ac], "-encode") == 0) {
			decode = false;
		}
		else if (files_provided == 0 && strcmp(argv[ac], "-decode") == 0) {
		}
		else if (strcmp(argv[ac], "-v") == 0) {
			logging.verbose = true;
		}
		else if (files_provided == 0 && strcmp(argv[ac], "-d") == 0 && ac + 1 < argc) {
			dirName = argv[ac + 1];
			files_provided++;
			ac++;
		}
		else if (decode && files_provided == 1 && strcmp(argv[ac], "-g") == 0 && ac + 1 < argc) {
			dirName = argv[ac + 1];
			files_provided++;
			ac++;
		}
		else if (decode && files_provided == 1 && strcmp(argv[ac], "-c") == 0) {
			cat = true;
			files_provided++;
		}
		else if (!decode && files_provided == 1 && strcmp(argv[ac], "-o") == 0 && ac + 1 < argc) {
			fileName = argv[ac + 1];
			files_provided++;
			ac++;
		}
		else if (files_provided == 0 && decode) {
			filesystem::path file_path = argv[ac];
			if (!filesystem::exists(file_path)) {
				cout << "MMC source file '" << argv[ac] << "' doesn't exist!\n";
				return;
			}
			fileName = file_path.string();
			files_provided++;
		}
		else if (files_provided == 0 && !decode) {
			filesystem::path dir_path = argv[ac];
			if (!filesystem::exists(dir_path)) {
				cout << "SSD source directory '" << argv[ac] << "' doesn't exist!\n";
				return;
			}
			dirName = dir_path.string();
			files_provided++;
		} else {
			cout << "Unknown option " << argv[ac] << "\n";
			printUsage(argv[0]);
			return;
		}
		ac++;
	}

	mParseSuccess = true;
}
