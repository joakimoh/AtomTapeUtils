
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "../shared/CommonTypes.h"
#include "ArgParser.h"
#include "../shared/TAPCodec.h"
#include "../shared/AtomBasicCodec.h"
#include "../shared/Logging.h"

using namespace std;
using namespace std::filesystem;



/*
 *
 * Create DATA file from TAP file
 * *
 */
int main(int argc, const char* argv[])
{


    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    if (arg_parser.logging.verbose)
        cout << "Output file name = " << arg_parser.dstFileName << "\n";

    TAPCodec TAP_codec = TAPCodec(arg_parser.logging, arg_parser.targetMachine);
    TapeFile TAP_file;
    if (!TAP_codec.decode(arg_parser.srcFileName, TAP_file)) {
        printf("Failed to decode TAP file '%s'\n", arg_parser.srcFileName.c_str());
    }


    AtomBasicCodec ABC_codec = AtomBasicCodec(arg_parser.logging, ACORN_ATOM);

    if (!ABC_codec.detokenise(TAP_file, arg_parser.dstFileName)) {
        printf("Failed to encode TAP file '%s' as BASIC program source file '%s'\n",
            arg_parser.srcFileName.c_str(), arg_parser.dstFileName.c_str()
        );
    }

    return 0;
}



