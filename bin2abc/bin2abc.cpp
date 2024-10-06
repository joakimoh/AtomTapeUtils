
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>
#include <cstdint>

#include <math.h>
#include "../shared/CommonTypes.h"
#include "ArgParser.h"
#include "../shared/AtomBasicCodec.h"
#include "../shared/Logging.h"

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

    if (arg_parser.logging.verbose)
        cout << "Output file name = " << arg_parser.dstFileName << "\n";

    // Open the input file
    ifstream fin(arg_parser.srcFileName, ios::in | ios::binary | ios::ate);
    if (!fin) {
        cout << "can't open file " << arg_parser.dstFileName << "\n";
        return false;
    }

    // Create vector the same size as the file
    auto fin_sz = fin.tellg();
    vector<uint8_t> data(fin_sz);


    // Fill the vector with the file content
    uint8_t* data_p = &data.front();
    fin.seekg(0);
    fin.read((char*)data_p, (streamsize)fin_sz);
    fin.close();


    AtomBasicCodec ABC_codec = AtomBasicCodec(arg_parser.logging, arg_parser.targetMachine);

    TapeFile TAP_file(arg_parser.targetMachine);


    if (!ABC_codec.decode(data, arg_parser.dstFileName)) {
        printf("Failed to decode binary program file '%s' into a readable program text file\n", arg_parser.dstFileName.c_str());
    }


    return 0;
}



