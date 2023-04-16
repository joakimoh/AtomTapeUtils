
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
#include "../shared/AtomBasicCodec.h"
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

    cout << "Output file is: '" << arg_parser.mDstFileName << "'\n";

    DataCodec DATA_codec = DataCodec();

    if (!DATA_codec.decode(arg_parser.mSrcFileName)) {
        DBG_PRINT(ERR, "Failed to encode DATA file '%s' as WAW file '%s'\n",
            arg_parser.mSrcFileName.c_str(), arg_parser.mDstFileName.c_str()
        );
    }
    cout << "DATA file decoded successfully...\n";

    TAPFile TAP_file;

    DATA_codec.getTAPFile(TAP_file);

    WavEncoder WAV_encoder = WavEncoder(TAP_file, arg_parser.mBaudrate);

    cout << "WAV encoder created...\n";

    if (!WAV_encoder.encode(arg_parser.mDstFileName)) {
        cout << "Failed to encode DATA file '" << arg_parser.mSrcFileName << "' as WAV file '" << arg_parser.mDstFileName << "'\n";
    }
    cout << "WAVE file created...\n";

    return 0;
}


