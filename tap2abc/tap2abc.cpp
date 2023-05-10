
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
#include "../shared/Debug.h"

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

    cout << "Output file name = " << arg_parser.dstFileName << "\n";

    TAPCodec TAP_codec = TAPCodec(arg_parser.verbose);

    if (!TAP_codec.decode(arg_parser.srcFileName)) {
        printf("Failed to decode TAP file '%s'\n", arg_parser.srcFileName.c_str());
    }

    TAPFile TAP_file;

    TAP_codec.getTAPFile(TAP_file);

    AtomBasicCodec ABC_codec = AtomBasicCodec(TAP_file, arg_parser.verbose);

    if (!ABC_codec.encode(arg_parser.dstFileName)) {
        printf("Failed to encode TAP file '%s' as ABC file '%s'\n",
            arg_parser.srcFileName.c_str(), arg_parser.dstFileName.c_str()
        );
    }

    return 0;
}



