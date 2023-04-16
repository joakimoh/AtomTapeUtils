
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "../shared/CommonTypes.h"
#include "ArgParser.h"
#include "../shared/UEFCodec.h"
#include "../shared/AtomBasicCodec.h"
#include "../shared/DataCodec.h"
#include "../shared/Compress.h"
#include "../shared/Debug.h"

using namespace std;
using namespace std::filesystem;



/*
 * 
 * Create UEF from Acorn Atom BASIC (ABC) program
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

    UEFCodec UEF_codec = UEFCodec(TAP_file);

    if (!UEF_codec.encode(arg_parser.mDstFileName)) {
        DBG_PRINT(ERR, "Failed to encode program file '%s' as UEF file '%s'\n",
            arg_parser.mSrcFileName.c_str(), arg_parser.mDstFileName.c_str()
        );
    }

    return 0;
}



