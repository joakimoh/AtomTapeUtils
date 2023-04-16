#include <gzstream.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>
#include "Debug.h"

using namespace std;

bool compressFile(string inFilename, string outFileName)
{
    ogzstream  out(outFileName.c_str());
    if (!out.good()) {
        DBG_PRINT(ERR, "Failed to create file %s for compression output\n", outFileName.c_str());
        return false;
    }
    std::ifstream in(inFilename);
    if (!in.good()) {
        DBG_PRINT(ERR, "Failed to open file %s for compression input\n", inFilename.c_str())
        return false;
    }
    char c;
    while (in.get(c))
        out << c;
    in.close();
    out.close();

    if (!in.eof()) {
        DBG_PRINT(ERR, "Failed reading file %s for compression input\n", inFilename.c_str())
        return false;
    }
    if (!out.good()) {
        DBG_PRINT(ERR, "Failed writing to file %s for compression output\n", outFileName.c_str());
        return false;
    }

    return true;
}

bool uncompressFile(string inFilename, string outFileName)
{
    igzstream in(inFilename.c_str());
    if (!in.good()) {
        DBG_PRINT(ERR, "Failed to open compressed input file %s\n", inFilename.c_str());
        return false;
    }
    std::ofstream out(outFileName);
    if (!out.good()) {
        DBG_PRINT(ERR, "Failed to create output file %s\n", outFileName.c_str());
        return false;
    }
    char c;
    while (in.get(c))
        out << c;
    in.close();
    out.close();
    if (!in.eof()) {
        DBG_PRINT(ERR, "Failed while reading compressed input file %s\n", inFilename.c_str());
        return false;
    }
    if (!out.good()) {
        DBG_PRINT(ERR, "Failed while writing to output file %s\n", outFileName.c_str());
        return false;
    }

    return true;
}