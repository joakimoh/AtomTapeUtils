
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
#include "../shared/MMCCodec.h"
#include "../shared/DataCodec.h"
#include "../shared/Compress.h"
#include "../shared/Debug.h"

using namespace std;
using namespace std::filesystem;



/*
 *
 * Create Atom MMC file from Acorn Atom BASIC (ABC) program
 *
 * The Atom MMC file format is used by Atomulator as well as the
 * AtoMMC2 & AtoMMC v4 SD/MMC card interface for storing native
 * Acorn Atom files onto an SC/MMC card.
 *
 * Format: <file name: 16> <load addr: 2> <exec addr: 2> <len: 2> <data>
 *
 * Where file name is max 13 chars and right-padded with zeros and
 * addresses and len are stored in little-endian order.
 *
 * See: http://www.acornatom.nl/atom_plaatjes/sd-files/atommmc2.html
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

    MMCCodec MMC_codec(TAP_file);

    if (!MMC_codec.encode(arg_parser.mDstFileName)) {
        DBG_PRINT(ERR, "Failed to encode DATA file '%s' as MMC file '%s'\n",
            arg_parser.mSrcFileName.c_str(), arg_parser.mDstFileName.c_str()
        );
    }

    return 0;
}



