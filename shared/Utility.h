#pragma once

#ifndef UTILITY_H
#define UTILITY_H

#include <cstdint>
#include "CommonTypes.h"
#include "FileBlock.h"
#include "TapeProperties.h"

class Utility {

public:
   
    static string getFileExt(string filePath);

    static bool move2FilePos(ifstream& fin, streampos pos);
    static bool readBytes(ifstream &fin, Byte* bytes, int n);
    static bool readBytes(ifstream &fin, streampos pos, Byte* bytes, int n);
    static bool readBytes(BytesIter &bytesInIter, Bytes &bytesIn, Bytes &bytesOut, int n);
    static bool readBytes(BytesIter& bytesInIter, Bytes &bytesIn, Byte* bytesOut, int n);
    static string paddedByteArray2String(Byte* a, int n);

    static void copybytes(Byte* from, Byte* to, int n);
    static void copybytes(Byte* from, Bytes& to, int n);
    static void initbytes(Byte* bytes, Byte v, int n);
    static void initbytes(Bytes& bytes, Byte v, int n);

    static uint32_t bytes2uint(Byte* bytes, int n, bool littleEndian);
    static void uint2bytes(uint32_t u, Byte* bytes, int n, bool littleEndian);

    static string crReadableString(string s, int n);
    static string crDefaultOutFileName(string filePath);
    static string crDefaultOutFileName(string filePath, string fileExt);
    static string crEncodedFileNamefromDir(string dirPath, TapeFile &tapeFile, string fileExt);
    static string crEncodedProgramFileNamefromDir(string dirPath, TargetMachine targetMachine, TapeFile &tapeFile);

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


