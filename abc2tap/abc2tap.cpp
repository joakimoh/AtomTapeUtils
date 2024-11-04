
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
#include "../shared/TAPCodec.h"
#include "../shared/Compress.h"
#include "../shared/Logging.h"

using namespace std;
using namespace std::filesystem;



/*
 *
 * Create Atom TAP/MMC file from an Acorn Atom/BBC Micro BASIC source program file.
 * 
 * The TAP/MMC file format is used by Atomulator as well as the
 * AtoMMC2 & AtoMMC v4 SD/MMC card interface for storing native
 * Acorn Atom files onto an SC/MMC card.
 * 
 * Format: <file name: 16> <load addr: 2> <exec addr: 2> <len: 2> <data>
 * 
 * Where file name is max 13 chars (Atom) or 10 chars (BBC Micro) and right-padded with zeros and
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

    if (arg_parser.logging.verbose)
        cout << "Output file name = " << arg_parser.dstFileName << "\n";

    AtomBasicCodec ABC_codec = AtomBasicCodec(arg_parser.logging, ACORN_ATOM);
    TapeFile TAP_file(ACORN_ATOM);
    if (!ABC_codec.tokenise(arg_parser.srcFileName, TAP_file)) {
        printf("Failed to decode program file '%s'\n", arg_parser.srcFileName.c_str());
    }


    TAPCodec TAP_codec(arg_parser.logging, arg_parser.targetMachine);
    if (!TAP_codec.encode(TAP_file, arg_parser.dstFileName)) {
        printf("Failed to encode program file '%s' as TAP/MMC file '%s'\n",
            arg_parser.srcFileName.c_str(), arg_parser.dstFileName.c_str()
        );
    }

    return 0;
}



