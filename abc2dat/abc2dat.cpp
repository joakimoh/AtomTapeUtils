
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "../shared/CommonTypes.h"
#include "ArgParser.h"
#include "../shared/AtomBasicCodec.h"
#include "../shared/DataCodec.h"
#include "../shared/Debug.h"

using namespace std;
using namespace std::filesystem;



/*
 *
 * Create DATA file from Acorn Atom BASIC (ABC) program
 * *
 */
int main(int argc, const char* argv[])
{


    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    if (arg_parser.verbose)
        cout << "Output file name = " << arg_parser.dstFileName << "\n";

    AtomBasicCodec ABC_codec = AtomBasicCodec(arg_parser.verbose);

    if (!ABC_codec.decode(arg_parser.srcFileName)) {
        printf("Failed to decode program file '%s'\n", arg_parser.srcFileName.c_str());
    }

    TAPFile TAP_file;

    ABC_codec.getTAPFile(TAP_file);

    DataCodec DATA_codec = DataCodec(TAP_file, arg_parser.verbose);

    if (!DATA_codec.encode(arg_parser.dstFileName)) {
        printf("Failed to encode program file '%s' as DATA file '%s'\n",
            arg_parser.srcFileName.c_str(), arg_parser.dstFileName.c_str()
        );
    }

    return 0;
}



