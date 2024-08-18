
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
#include "../shared/DataCodec.h"

using namespace std;
using namespace std::filesystem;



/*
 * 
 * Create a DATA file from UEF file
 * * 
 */
int main(int argc, const char* argv[])
{


    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    UEFCodec UEF_codec = UEFCodec(arg_parser.verbose, arg_parser.bbcMicro);
    TapeFile TAP_file(AtomFile); // Set to Atom initiall, but could be changed to BBC Micro by coded!
    if (!UEF_codec.decode(arg_parser.srcFileName, TAP_file)) {
        cout << "Failed to decode UEF file '" << arg_parser.srcFileName << "'\n";
    }

    DataCodec DATA_codec = DataCodec(arg_parser.verbose);

    if (!DATA_codec.encode(TAP_file, arg_parser.dstFileName)) {
        
        cout << "Failed to encode UEF file '" << arg_parser.srcFileName << "' as DATA file '" << arg_parser.dstFileName << "'\n";
    }

    return 0;
}



