
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "ArgParser.h"
#include "../shared/AtomBasicCodec.h"
#include "../shared/TAPCodec.h"
#include "../shared/Logging.h"

using namespace std;
using namespace std::filesystem;



/*
 *
 * Create Binary file from Program file
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
    TapeFile tape_file(arg_parser.targetMachine);
    if (!ABC_codec.tokenise(arg_parser.srcFileName, arg_parser.dstFileName)) {
        printf("Failed to decode program file '%s'\n", arg_parser.srcFileName.c_str());
        return false;
    }

    return 0;
}



