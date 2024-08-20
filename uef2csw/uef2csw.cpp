
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
#include "../shared/CSWCodec.h"


using namespace std;
using namespace std::filesystem;



/*
 * 
 * Create Acorn Atom CSW file from UEF file
 * 
 * 
 */
int main(int argc, const char* argv[])
{
    

    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    UEFCodec UEF_codec = UEFCodec(arg_parser.verbose, arg_parser.bbcMicro);

    TapeFile TAP_file(AtomFile);
    if (arg_parser.bbcMicro)
        TAP_file = TapeFile(BBCMicroFile);

    if (!UEF_codec.decode(arg_parser.srcFileName, TAP_file)) {
        cout << "Failed to decode UEF file '" << arg_parser.srcFileName << "'\n";
    }



    CSWCodec CSW_codec = CSWCodec(arg_parser.mPreserveOriginalTiming, arg_parser.verbose);

    if (!CSW_codec.encode(TAP_file, arg_parser.dstFileName, arg_parser.mSampleFreq)) {
        
        cout << "Failed to encode UEF file '" << arg_parser.srcFileName << "' as CSW file '" << arg_parser.dstFileName << "'\n";
    }



    

    return 0;
}



