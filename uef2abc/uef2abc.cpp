
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

using namespace std;
using namespace std::filesystem;



/*
 * 
 * Create  Acorn Atom BASIC (ABC) program from UEF
 * * 
 */
int main(int argc, const char* argv[])
{


    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    UEFCodec UEF_codec = UEFCodec(arg_parser.verbose, arg_parser.targetMachine);
    TapeFile TAP_file(ACORN_ATOM);
    if (!UEF_codec.decode(arg_parser.srcFileName, TAP_file)) {
        cout << "Failed to decode UEF file '" << arg_parser.srcFileName << "'\n";
    }

    AtomBasicCodec ABC_codec = AtomBasicCodec(arg_parser.verbose, arg_parser.targetMachine);
    if (!ABC_codec.encode(TAP_file, arg_parser.dstFileName)) {
        
        cout << "Failed to encode UEF file '" << arg_parser.srcFileName << "' as ABC file '" << arg_parser.dstFileName << "'\n";
    }



    

    return 0;
}



