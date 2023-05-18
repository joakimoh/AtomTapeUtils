
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
#include "../shared/Debug.h"

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

    if (arg_parser.verbose)
        cout << "Output file name = " << arg_parser.dstFileName << "\n";

    DataCodec DATA_codec = DataCodec(arg_parser.verbose);

    if (!DATA_codec.decode(arg_parser.srcFileName)) {
        printf("Failed to decode DATA file '%s'\n", arg_parser.srcFileName.c_str());
    }

    TAPFile TAP_file;

    DATA_codec.getTAPFile(TAP_file);

    AtomBasicCodec ABC_codec = AtomBasicCodec(TAP_file, arg_parser.verbose);

    if (!ABC_codec.encode(arg_parser.dstFileName)) {
        printf("Failed to encode DATA file '%s' as ABC file '%s'\n",
            arg_parser.srcFileName.c_str(), arg_parser.dstFileName.c_str()
        );
    }

    return 0;
}



