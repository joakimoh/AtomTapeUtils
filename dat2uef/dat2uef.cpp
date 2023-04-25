
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "ArgParser.h"
#include "../shared/DataCodec.h"
#include "../shared/UEFCodec.h"
#include "../shared/TAPCodec.h"
#include "../shared/Debug.h"

using namespace std;
using namespace std::filesystem;



/*
 *
 * Create UEF file from DATA file
 * *
 */
int main(int argc, const char* argv[])
{


    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    cout << "Output file name = " << arg_parser.mDstFileName << "\n";

    DataCodec DATA_codec = DataCodec();
    int load_address;
    Bytes data;
    if (!DATA_codec.decode(arg_parser.mSrcFileName)) {
        DBG_PRINT(ERR, "Failed to decode program file '%s'\n", arg_parser.mSrcFileName.c_str());
        return false;
    }

    TAPFile TAP_file;

    DATA_codec.getTAPFile(TAP_file);


    UEFCodec UEF_codec = UEFCodec(TAP_file, false);
    UEF_codec.setTapeTiming(arg_parser.tapeTiming);

    if (!UEF_codec.encode(arg_parser.mDstFileName)) {
        DBG_PRINT(ERR, "Failed to encode DATA file '%s' as UEF file '%s'\n",
            arg_parser.mSrcFileName.c_str(), arg_parser.mDstFileName.c_str()
        );
    }

    

    return 0;
}



