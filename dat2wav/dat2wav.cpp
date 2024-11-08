
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
#include "../shared/Logging.h"
#include "../shared/Utility.h"

using namespace std;
using namespace std::filesystem;



/*
 *
 * Create WAV file from a BASIC program
 * *
 */
int main(int argc, const char* argv[])
{


    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    if (arg_parser.logging.verbose)
        cout << "Output file is: '" << arg_parser.dstFileName << "'\n";

    DataCodec DATA_codec = DataCodec(arg_parser.logging);

    TapeFile TAP_file(arg_parser.targetMachine);

    if (!DATA_codec.decode(arg_parser.srcFileName, arg_parser.targetMachine, TAP_file)) {
        printf("Failed to encode DATA file '%s' as WAW file '%s'\n",
            arg_parser.srcFileName.c_str(), arg_parser.dstFileName.c_str()
        );
    }

    int sample_freq = 44100;
    WavEncoder WAV_encoder = WavEncoder(sample_freq, arg_parser.tapeTiming, arg_parser.logging, arg_parser.targetMachine);
    if (!WAV_encoder.encode(TAP_file, arg_parser.dstFileName)) {
        cout << "Failed to encode DATA file '" << arg_parser.srcFileName << "' as WAV file '" << arg_parser.dstFileName << "'\n";
    }

    return 0;
}



