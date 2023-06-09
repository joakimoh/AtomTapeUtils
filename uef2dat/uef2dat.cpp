﻿
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

    UEFCodec UEF_codec = UEFCodec(arg_parser.verbose);

    if (!UEF_codec.decode(arg_parser.srcFileName)) {
        cout << "Failed to decode UEF file '" << arg_parser.srcFileName << "'\n";
    }

    TAPFile TAP_file;

    UEF_codec.getTAPFile(TAP_file);

    DataCodec DATA_codec = DataCodec(TAP_file, arg_parser.verbose);

    if (!DATA_codec.encode(arg_parser.dstFileName)) {
        
        cout << "Failed to encode UEF file '" << arg_parser.srcFileName << "' as DATA file '" << arg_parser.dstFileName << "'\n";
    }

    return 0;
}



