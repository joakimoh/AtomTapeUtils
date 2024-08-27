#pragma once

#ifndef UTILITY_H
#define UTILITY_H

#include "CommonTypes.h"
#include "BlockTypes.h"

void logTAPFileHdr(TapeFile& tapeFile);
void logTAPFileHdr(ostream* fout, TapeFile& tapeFile);
void logTAPBlockHdr(FileBlock& block, uint32_t adr_offset);
void logTAPBlockHdr(ostream *fout, FileBlock& block, uint32_t adr_offset);

bool encodeTAPHdr(FileBlock& block, string tapefileName, uint32_t fileLoadAdr, uint32_t loadAdr, uint32_t execAdr, uint32_t blockNo, uint32_t BlockSz);

void copybytes(Byte* from, Byte* to, int n);
void copybytes(Byte* from, Bytes& to, int n);
void initbytes(Byte* bytes, Byte v, int n);
void initbytes(Bytes &bytes, Byte v, int n);

uint32_t bytes2uint(Byte* bytes, int n, bool littleEndian);
void uint2bytes(uint32_t u, Byte* bytes, int n, bool littleEndian);

// Read file name from tape block
bool readAtomTapeFileName(BytesIter& data_iter, Bytes& data, Word& CRC, char* name);
bool readBBMTapeFileName(BytesIter& data_iter, Bytes& data, Word& CRC, char* name);

// CRC calculation
void updateAtomCRC(Word& CRC, Byte data);
void updateBBMCRC(Word& CRC, Byte data);

string crDefaultOutFileName(string filePath);
string crDefaultOutFileName(string filePath, string fileExt);
string crEncodedFileNamefromDir(string dirPath, TapeFile tapeFile, string fileExt);

// Log continuous memory data starting from an assumed address
void logData(int address, BytesIter &data_iter, int data_sz);
void logData(int address, Byte* data, int sz);

BlockType parseBlockFlag(AtomTapeBlockHdr hdr, int& blockLen);

double decodeTime(string time);
string encodeTime(double time);

string atomBlockNameFromFilename(string filename);
string bbmBlockNameFromFilename(string fn);

string filenameFromBlockName(string blockName);

bool extractBlockPars(
    FileBlock block, BlockType blockType, int nReadChars,
    int& loadAdr, int& loadAdrUB, int& execAdr, int& blockSz,
    bool& isBasicProgram, string& fileName
);

bool extractBBMBlockPars(
    FileBlock block, bool is_last_block, int nReadChars,
    int& loadAdr, int& loadAdrUB, int& execAdr, int& blockSz,
    bool& isBasicProgram, string& fileName
);

#endif


