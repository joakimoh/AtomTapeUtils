
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
#include "../shared/Logging.h"
#include "../shared/Utility.h"
#include "../shared/TAPCodec.h"
#include "../shared/BinCodec.h"

using namespace std;
using namespace std::filesystem;



/*
 * 
 * Create UEF, DATA & TAP/MMC files from an Acorn Atom/BBC Micro BASIC source program file
 * * 
 */
int main(int argc, const char* argv[])
{

    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    if (arg_parser.logging.verbose)
        cout << "Output directory = " << arg_parser.dstDir << "\n";


    // Decode BASIC source file
    TapeFile TAP_file(ACORN_ATOM);
    AtomBasicCodec ABC_codec = AtomBasicCodec(arg_parser.logging, arg_parser.targetMachine);
    if (!ABC_codec.tokenise(arg_parser.srcFileName, TAP_file)) {
        printf("Failed to decode program file '%s'\n", arg_parser.srcFileName.c_str());
    }
    




    //
    // Generate files
    // 

    // Generate DATA file
    DataCodec DATA_codec = DataCodec(arg_parser.logging);
    string DATA_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, TAP_file, "dat");
    if (!DATA_codec.encode(TAP_file, DATA_file_name)) {
        cout << "Failed to write the DATA file!\n";
    }

    // Generate TAP file
    TAPCodec TAP_codec = TAPCodec(arg_parser.logging);
    string TAP_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, TAP_file, "tap");
    if (!TAP_codec.encode(TAP_file, TAP_file_name)) {
        cout << "Failed to write the TAP file!\n";
    }


    // generate UEF file
    UEFCodec UEF_codec = UEFCodec(false, arg_parser.logging, arg_parser.targetMachine);
    string UEF_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, TAP_file, "uef");
    if (!UEF_codec.encode(TAP_file, UEF_file_name)) {
        cout << "Failed to write the UEF file!\n";
    }

    // Create binary file
    string BIN_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, TAP_file, "");
    BinCodec BIN_codec(arg_parser.logging);
    if (!BIN_codec.encode(TAP_file, BIN_file_name)) {
        cout << "can't create Binary file " << BIN_file_name << "\n";
    }

    // Create INF file
    if (!BinCodec::generateInfFile(arg_parser.dstDir, TAP_file)) {
        cout << "Failed to write the INF file!\n";
        //return -1;
    }

    return 0;
}



