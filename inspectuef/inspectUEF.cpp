
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "../shared/CommonTypes.h"
#include "../shared/UEFCodec.h"
#include "../shared/Logging.h"
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

    Logging debug_info;
    UEFCodec UEF_codec = UEFCodec(debug_info, UNKNOWN_TARGET);
    
    // Create output file file
    ostream* fout_p = &cout;
    if (arg_parser.dstFileName != "") {
        fout_p = new ofstream(arg_parser.dstFileName);
        if (!*fout_p) {
            cout << "can't write t file " << arg_parser.dstFileName << "\n";
            return (-1);
        }
        UEF_codec.setStdOut(fout_p);
    }
 

    
    if (!UEF_codec.inspect(arg_parser.srcFileName)) {
        cout << "Failed to decode UEF file '" << arg_parser.srcFileName << "'\n";
    }

    if (arg_parser.dstFileName != "") {
        ((ofstream*)fout_p)->close();
        delete fout_p;
    }

    return 0;
}



