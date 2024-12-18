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
#include "../shared/AtomBasicCodec.h"
#include "../shared/DataCodec.h"
#include "../shared/Compress.h"
#include "../shared/Logging.h"

using namespace std;
using namespace std::filesystem;



/*
 * 
 * Create UEF from an Acorn Atom/BBC Micro BASIC source program file.
 * * 
 */
int main(int argc, const char* argv[])
{


    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    if (arg_parser.logging.verbose)
        cout << "Output file name = " << arg_parser.dstFileName << "\n";

    AtomBasicCodec ABC_codec = AtomBasicCodec(arg_parser.logging, arg_parser.targetMachine);

    TapeFile TAP_file(arg_parser.targetMachine);

    if (!ABC_codec.tokenise(arg_parser.srcFileName, TAP_file)) {
        printf("Failed to decode program file '%s'\n", arg_parser.srcFileName.c_str());
    }
 



    UEFCodec UEF_codec = UEFCodec(false, arg_parser.logging, arg_parser.targetMachine);
    UEF_codec.setTapeTiming(arg_parser.tapeTiming);


    if (!UEF_codec.encode(TAP_file, arg_parser.dstFileName)) {
        printf("Failed to encode program file '%s' as UEF file '%s'\n",
            arg_parser.srcFileName.c_str(), arg_parser.dstFileName.c_str()
        );
    }

    return 0;
}



