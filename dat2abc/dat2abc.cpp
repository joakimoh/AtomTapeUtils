
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

    cout << "Output file name = " << arg_parser.mDstFileName << "\n";

    DataCodec DATA_codec = DataCodec();

    if (!DATA_codec.decode(arg_parser.mSrcFileName)) {
        DBG_PRINT(ERR, "Failed to decode DATA file '%s'\n", arg_parser.mSrcFileName.c_str());
    }

    TAPFile TAP_file;

    DATA_codec.getTAPFile(TAP_file);

    AtomBasicCodec ABC_codec = AtomBasicCodec(TAP_file);

    if (!ABC_codec.encode(arg_parser.mDstFileName)) {
        DBG_PRINT(ERR, "Failed to encode DATA file '%s' as ABC file '%s'\n",
            arg_parser.mSrcFileName.c_str(), arg_parser.mDstFileName.c_str()
        );
    }

    return 0;
}



