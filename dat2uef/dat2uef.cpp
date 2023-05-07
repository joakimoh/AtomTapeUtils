
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

    cout << "Output file name = " << arg_parser.dstFileName << "\n";

    DataCodec DATA_codec = DataCodec(arg_parser.verbose);
    int load_address;
    Bytes data;
    if (!DATA_codec.decode(arg_parser.srcFileName)) {
        printf("Failed to decode program file '%s'\n", arg_parser.srcFileName.c_str());
        return false;
    }

    TAPFile TAP_file;

    DATA_codec.getTAPFile(TAP_file);


    UEFCodec UEF_codec = UEFCodec(TAP_file, false, arg_parser.verbose);
    UEF_codec.setTapeTiming(arg_parser.tapeTiming);

    if (!UEF_codec.encode(arg_parser.dstFileName)) {
        printf("Failed to encode DATA file '%s' as UEF file '%s'\n",
            arg_parser.srcFileName.c_str(), arg_parser.dstFileName.c_str()
        );
    }

    

    return 0;
}



