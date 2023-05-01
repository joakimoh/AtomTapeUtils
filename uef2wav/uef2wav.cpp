
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

    UEFCodec UEF_codec = UEFCodec();

    if (!UEF_codec.decode(arg_parser.mSrcFileName)) {
        cout << "Failed to decode UEF file '" << arg_parser.mSrcFileName << "'\n";
    }

    TAPFile TAP_file;

    UEF_codec.getTAPFile(TAP_file);

    WavEncoder WAV_encoder = WavEncoder(TAP_file, false, 44100);
    WAV_encoder.setTapeTiming(arg_parser.tapeTiming);

    cout << "WAV encoder created...\n";

    if (!WAV_encoder.encode(arg_parser.mDstFileName)) {
        cout << "Failed to encode program file '" << arg_parser.mSrcFileName << "' as WAV file '" << arg_parser.mDstFileName << "'\n";
    }
    cout << "WAVE file created...\n";

    return 0;
}



