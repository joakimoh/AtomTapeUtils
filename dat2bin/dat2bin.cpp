
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "ArgParser.h"
#include "../shared/DataCodec.h"
#include "../shared/Debug.h"

using namespace std;
using namespace std::filesystem;



/*
 *
 * Create Binary file from DATA file
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
    if (!DATA_codec.decode2Bytes(arg_parser.srcFileName, load_address, data)) {
        printf("Failed to decode program file '%s'\n", arg_parser.srcFileName.c_str());
        return false;
    }

    BytesIter data_iter = data.begin();

    // Create the output file
    ofstream fout(arg_parser.dstFileName, ios::out | ios::binary | ios::ate);
    if (!fout) {
        cout << "can't write to file " << arg_parser.dstFileName << "\n";
        return false;
    }

    while (data_iter < data.end()) {
        fout << *data_iter++;
    }
    fout.close();

    return 0;
}



