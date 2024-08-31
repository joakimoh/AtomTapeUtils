
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "../shared/CommonTypes.h"
#include "ArgParser.h"
#include "../shared/UEFCodec.h"
#include "../shared/AtomBasicCodec.h"
#include "../shared/DataCodec.h"
#include "../shared/Compress.h"
#include "../shared/Debug.h"
#include "../shared/Utility.h"
#include "../shared/TAPCodec.h"

using namespace std;
using namespace std::filesystem;



/*
 * 
 * Create UEF, DATA & TAP/MMC files from Acorn Atom BASIC (ABC) program
 * * 
 */
int main(int argc, const char* argv[])
{

    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    if (arg_parser.verbose)
        cout << "Output directory = " << arg_parser.dstDir << "\n";


    // Decode ABC file
    TapeFile TAP_file(ACORN_ATOM);
    AtomBasicCodec ABC_codec = AtomBasicCodec(arg_parser.verbose, arg_parser.targetMachine);
    if (!ABC_codec.decode(arg_parser.srcFileName, TAP_file)) {
        printf("Failed to decode program file '%s'\n", arg_parser.srcFileName.c_str());
    }
    




    //
    // Generate files
    // 

    // Generate DATA file
    DataCodec DATA_codec = DataCodec(arg_parser.verbose);
    string DATA_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, TAP_file, "dat");
    if (!DATA_codec.encode(TAP_file, DATA_file_name)) {
        cout << "Failed to write the DATA file!\n";
    }

    // Generate TAP file
    if (arg_parser.targetMachine == ACORN_ATOM) {
        TAPCodec TAP_codec = TAPCodec(arg_parser.verbose);
        string TAP_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, TAP_file, "tap");
        if (!TAP_codec.encode(TAP_file, TAP_file_name)) {
            cout << "Failed to write the TAP file!\n";
        }
    }

    // generate UEF file
    UEFCodec UEF_codec = UEFCodec(false, arg_parser.verbose, arg_parser.targetMachine);
    string UEF_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, TAP_file, "uef");
    if (!UEF_codec.encode(TAP_file, UEF_file_name)) {
        cout << "Failed to write the UEF file!\n";
    }

    // Create binary file
    string BIN_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, TAP_file, "");
    if (!TAPCodec::data2Binary(TAP_file, BIN_file_name)) {
        cout << "can't create Binary file " << BIN_file_name << "\n";
    }

    return 0;
}



