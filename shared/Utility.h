#pragma once

#ifndef UTILITY_H
#define UTILITY_H

#include <cstdint>
#include "CommonTypes.h"
#include "FileBlock.h"
#include "TapeProperties.h"

class Utility {

public:

    static void copybytes(Byte* from, Byte* to, int n);
    static void copybytes(Byte* from, Bytes& to, int n);
    static void initbytes(Byte* bytes, Byte v, int n);
    static void initbytes(Bytes& bytes, Byte v, int n);

    static uint32_t bytes2uint(Byte* bytes, int n, bool littleEndian);
    static void uint2bytes(uint32_t u, Byte* bytes, int n, bool littleEndian);

    static string crDefaultOutFileName(string filePath);
    static string crDefaultOutFileName(string filePath, string fileExt);
    static string crEncodedFileNamefromDir(string dirPath, TapeFile tapeFile, string fileExt);

    // Log continuous memory data starting from an assumed address
    static void logData(int address, BytesIter& data_iter, int data_sz);
    static void logData(int address, Byte* data, int sz);
    static void logData(ostream *fout, int address, BytesIter& data_iter, int data_sz);
    static void logData(ostream *fout, int address, Byte* data, int sz);

    static double decodeTime(string time);
    static string encodeTime(double time);

    static string roundedPD(string prefix, double d, string suffix);

    static char digitToHex(int d);


};

#endif


