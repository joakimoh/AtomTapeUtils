
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

    cout << "Output file name = " << arg_parser.mDstFileName << "\n";

    AtomBasicCodec ABC_codec = AtomBasicCodec();

    if (!ABC_codec.decode(arg_parser.mSrcFileName)) {
        DBG_PRINT(ERR, "Failed to decode program file '%s'\n", arg_parser.mSrcFileName.c_str());
    }

    TAPFile TAP_file;

    ABC_codec.getTAPFile(TAP_file);

    DataCodec DATA_codec = DataCodec(TAP_file);

    if (!DATA_codec.encode(arg_parser.mDstFileName)) {
        DBG_PRINT(ERR, "Failed to encode program file '%s' as DATA file '%s'\n",
            arg_parser.mSrcFileName.c_str(), arg_parser.mDstFileName.c_str()
        );
    }

    return 0;
}



