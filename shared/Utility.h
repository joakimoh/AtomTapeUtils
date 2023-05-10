#pragma once

#ifndef UTILITY_H
#define UTILITY_H

#include "CommonTypes.h"
#include "BlockTypes.h"

string crDefaultOutFileName(string filePath);
string crDefaultOutFileName(string filePath, string fileExt);
string crEncodedFileNamefromDir(string dirPath, TAPFile mTapFile, string fileExt);

void logData(int address, BytesIter data_iter, int data_sz);

BlockType parseBlockFlag(AtomTapeBlockHdr hdr, int& blockLen);

double decodeTime(string time);
string encodeTime(double time);

string blockNameFromFilename(string filename);

string filenameFromBlockName(string blockName);

bool extractBlockPars(
    ATMBlock block, BlockType blockType, int nReadChars,
    int& loadAdr, int& loadAdrUB, int& execAdr, int& blockSz,
    bool& isAbcProgram, string& fileName
);

#endif


