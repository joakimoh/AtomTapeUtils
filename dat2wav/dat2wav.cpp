
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "../shared/CommonTypes.h"
#include "ArgParser.h"
#include "../shared/WavEncoder.h"
#include "../shared/DataCodec.h"
#include "../shared/Debug.h"
#include "../shared/Utility.h"

using namespace std;
using namespace std::filesystem;



/*
 *
 * Create WAV file from Acorn Atom BASIC (ABC) program
 * *
 */
int main(int argc, const char* argv[])
{


    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    if (arg_parser.verbose)
        cout << "Output file is: '" << arg_parser.dstFileName << "'\n";

    DataCodec DATA_codec = DataCodec(arg_parser.verbose);

    if (!DATA_codec.decode(arg_parser.srcFileName)) {
        printf("Failed to encode DATA file '%s' as WAW file '%s'\n",
            arg_parser.srcFileName.c_str(), arg_parser.dstFileName.c_str()
        );
    }

    TAPFile TAP_file;

    DATA_codec.getTAPFile(TAP_file);

    WavEncoder WAV_encoder = WavEncoder(TAP_file, false, 44100, arg_parser.verbose);
    WAV_encoder.setTapeTiming(arg_parser.tapeTiming);


    if (!WAV_encoder.encode(arg_parser.dstFileName)) {
        cout << "Failed to encode DATA file '" << arg_parser.srcFileName << "' as WAV file '" << arg_parser.dstFileName << "'\n";
    }

    return 0;
}



