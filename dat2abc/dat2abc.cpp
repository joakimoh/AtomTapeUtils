
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "../shared/CommonTypes.h"
#include "ArgParser.h"
#include "../shared/DataCodec.h"
#include "../shared/AtomBasicCodec.h"
#include "../shared/Logging.h"

using namespace std;
using namespace std::filesystem;



/*
 *
 * Create DATA file from atoMMC file
 * *
 */
int main(int argc, const char* argv[])
{


    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    if (arg_parser.logging.verbose)
        cout << "Output file name = " << arg_parser.dstFileName << "\n";

    DataCodec DATA_codec = DataCodec(arg_parser.logging);

    TapeFile TAP_file(arg_parser.targetMachine);

    if (!DATA_codec.decode(arg_parser.srcFileName, arg_parser.targetMachine, TAP_file)) {
        printf("Failed to decode DATA file '%s'\n", arg_parser.srcFileName.c_str());
    }

    AtomBasicCodec ABC_codec = AtomBasicCodec(arg_parser.logging, arg_parser.targetMachine);
    if (!ABC_codec.detokenise(TAP_file, arg_parser.dstFileName)) {
        printf("Failed to encode DATA file '%s' as ABC/BBC file '%s'\n",
            arg_parser.srcFileName.c_str(), arg_parser.dstFileName.c_str()
        );
    }

    return 0;
}



