
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
#include "../shared/MMCCodec.h"

using namespace std;
using namespace std::filesystem;



/*
 * 
 * Create UEF from Acorn Atom BASIC (ABC) program
 * * 
 */
int main(int argc, const char* argv[])
{

    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    cout << "Output directory = " << arg_parser.mDstDir << "\n";


    // Decode ABC file
    AtomBasicCodec ABC_codec = AtomBasicCodec();
    if (!ABC_codec.decode(arg_parser.mSrcFileName)) {
        DBG_PRINT(ERR, "Failed to decode program file '%s'\n", arg_parser.mSrcFileName.c_str());
    }
    TAPFile TAP_file;
    ABC_codec.getTAPFile(TAP_file);



    //
    // Generate files
    // 

    // Generate DATA file
    DataCodec DATA_codec = DataCodec(TAP_file);
    string DATA_file_name = crEncodedFileNamefromDir(arg_parser.mDstDir, TAP_file, "dat");
    if (!DATA_codec.encode(DATA_file_name)) {
        cout << "Failed to write the DATA file!\n";
    }

    // Generate TAP file
    TAPCodec TAP_codec = TAPCodec(TAP_file);
    string TAP_file_name = crEncodedFileNamefromDir(arg_parser.mDstDir, TAP_file, "tap");
    if (!TAP_codec.encode(TAP_file_name)) {
        cout << "Failed to write the TAP file!\n";
    }

    // generate UEF file
    UEFCodec UEF_codec = UEFCodec(TAP_file, false);
    string UEF_file_name = crEncodedFileNamefromDir(arg_parser.mDstDir, TAP_file, "uef");
    if (!UEF_codec.encode(UEF_file_name)) {
        cout << "Failed to write the UEF file!\n";
    }

    // Generate MMC file
    MMCCodec MMC_codec(TAP_file);
    string MMC_file_name = crEncodedFileNamefromDir(arg_parser.mDstDir, TAP_file, "");
    if (!MMC_codec.encode(MMC_file_name)) {
        cout << "Failed to write the MMC file!\n";
    }


    return 0;
}



