
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
#include "../shared/UEFCodec.h"
#include "../shared/DataCodec.h"
#include "../shared/Debug.h"
#include "../shared/Utility.h"

using namespace std;
using namespace std::filesystem;



/*
 *
 * Create WAV file from UEF file
 * *
 */
int main(int argc, const char* argv[])
{


    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    UEFCodec UEF_codec = UEFCodec(arg_parser.verbose, arg_parser.bbcMicro);

    TapeFile TAP_file(AtomFile);
    if (arg_parser.bbcMicro)
        TAP_file = TapeFile(BBCMicroFile);

    if (!UEF_codec.decode(arg_parser.srcFileName, TAP_file)) {
        cout << "Failed to decode UEF file '" << arg_parser.srcFileName << "'\n";
        return -1;
    }

    WavEncoder WAV_encoder = WavEncoder(arg_parser.mPreserveOriginalTiming, arg_parser.mSampleFreq, arg_parser.verbose, arg_parser.bbcMicro);
    WAV_encoder.setTapeTiming(arg_parser.tapeTiming);
    if (!WAV_encoder.encode(TAP_file, arg_parser.dstFileName)) {
        cout << "Failed to encode program file '" << arg_parser.srcFileName << "' as WAV file '" << arg_parser.dstFileName << "'\n";
    }

    return 0;
}



