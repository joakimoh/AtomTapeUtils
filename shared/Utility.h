#pragma once

#ifndef UTILITY_H
#define UTILITY_H

#include "CommonTypes.h"
#include "BlockTypes.h"

bool setBitTiming(int baudRate, bool bbcMicro, int& startBitCycles, int& lowDataBitCycles, int& highDataBitCycles, int& stopBitCycles);

void logAtomTapeHdr(AtomTapeBlockHdr &hdr, char blockName[]);
void logAtomTapeHdr(ostream* fout, AtomTapeBlockHdr &hdr, char blockName[]);

void logBBMTapeHdr(BBMTapeBlockHdr& hdr, char blockName[]);
void logBBMTapeHdr(ostream* fout, BBMTapeBlockHdr& hdr, char blockName[]);


void logTAPFileHdr(TapeFile& tapeFile);
void logTAPFileHdr(ostream* fout, TapeFile& tapeFile);
void logTAPBlockHdr(FileBlock& block, uint32_t adr_offset, uint32_t blockNo = 0);
void logTAPBlockHdr(ostream *fout, FileBlock& block, uint32_t adr_offset, uint32_t blockNo = 0);

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

BlockType parseAtomTapeBlockFlag(AtomTapeBlockHdr hdr, int& blockLen);

double decodeTime(string time);
string encodeTime(double time);

string atomBlockNameFromFilename(string filename);
string bbmBlockNameFromFilename(string fn);

string filenameFromBlockName(string blockName);

bool decodeATMBlockHdr(
    FileBlock block, BlockType blockType, int nReadChars,
    int& loadAdr, int& loadAdrUB, int& execAdr, int& blockSz,
    bool& isBasicProgram, string& fileName
);

bool decodeBTMBlockHdr(
    FileBlock block, bool is_last_block, int nReadChars,
    int& loadAdr, int& execAdr, int& blockSz,
    bool& isBasicProgram, string& fileName
);

bool decodeAtomTapeHdr(AtomTapeBlockHdr& hdr, FileBlock& block,
    int& loadAdr, int& execAdr, int& blockLen, int& blockNo, BlockType &blockType
);

bool decodeBBMTapeHdr(BBMTapeBlockHdr& hdr, FileBlock& block,
    uint32_t &loadAdr, uint32_t &execAdr, uint32_t &blockLen, int &blockNo, uint32_t &nextAdr, bool &isLastBlock
);

bool encodeTapeHdr(FileBlock& block, int block_no, int n_blocks, Bytes& hdr);
bool encodeAtomTapeHdr(FileBlock& block, int block_no, int n_blocks, Bytes& hdr);
bool encodeBBMTapeHdr(FileBlock& block, Bytes& hdr);

#endif


