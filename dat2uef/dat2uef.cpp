
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

    if (arg_parser.verbose)
        cout << "Output file name = " << arg_parser.dstFileName << "\n";

    DataCodec DATA_codec = DataCodec(arg_parser.verbose);
    int load_address;
    Bytes data;
    TapeFile TAP_file(AtomFile);
    if (arg_parser.bbcMicro)
        TAP_file = TapeFile(BBCMicroFile);

    if (!DATA_codec.decode(arg_parser.srcFileName, TAP_file, arg_parser.bbcMicro)) {
        printf("Failed to decode program file '%s'\n", arg_parser.srcFileName.c_str());
        return false;
    }



    UEFCodec UEF_codec = UEFCodec(false, arg_parser.verbose, arg_parser.bbcMicro);
    UEF_codec.setTapeTiming(arg_parser.tapeTiming);
    if (!UEF_codec.encode(TAP_file, arg_parser.dstFileName)) {
        printf("Failed to encode DATA file '%s' as UEF file '%s'\n",
            arg_parser.srcFileName.c_str(), arg_parser.dstFileName.c_str()
        );
    }

    

    return 0;
}



