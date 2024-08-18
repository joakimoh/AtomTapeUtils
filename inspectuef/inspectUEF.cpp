
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "../shared/CommonTypes.h"
#include "../shared/UEFCodec.h"
#include "ArgParser.h"

using namespace std;
using namespace std::filesystem;



/*
 *
 * Decode and display content of UEF file
 *
 */
int main(int argc, const char* argv[])
{

    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;
 

    UEFCodec UEF_codec = UEFCodec(true, false);
    if (!UEF_codec.inspect(arg_parser.srcFileName)) {
        cout << "Failed to decode UEF file '" << arg_parser.srcFileName << "'\n";
    }

    return 0;
}



