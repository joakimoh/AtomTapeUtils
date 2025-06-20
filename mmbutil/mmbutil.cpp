
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "ArgParser.h"
#include "../shared/MMBCodec.h"


using namespace std;
using namespace std::filesystem;



/*
 *
 * Utility to decode & encode MMB files
 *
 *
 */
int main(int argc, const char* argv[])
{


    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    MMBCodec MMB_codec = MMBCodec(arg_parser.logging);

    if (arg_parser.decode)
        MMB_codec.decode(arg_parser.fileName, arg_parser.dirName, arg_parser.cat);
    else
        MMB_codec.encode(arg_parser.dirName, arg_parser.fileName);


    return 0;
}



