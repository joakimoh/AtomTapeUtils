#pragma once

#ifndef UTILITY_H
#define UTILITY_H

#include "CommonTypes.h"
#include "BlockTypes.h"
#include "TapeProperties.h"

class Utility {

public:

    static void logTapeTiming(TapeProperties tapeTiming);

    static bool initTAPBlock(TargetMachine blockType, FileBlock& block);
    static bool encodeTAPHdr(FileBlock& block, string tapefileName, uint32_t fileLoadAdr, uint32_t loadAdr, uint32_t execAdr, uint32_t blockNo, uint32_t BlockSz);

    static bool setBitTiming(int baudRate, TargetMachine targetMachine, int& startBitCycles, int& lowDataBitCycles, int& highDataBitCycles, int& stopBitCycles);

    static void logAtomTapeHdr(AtomTapeBlockHdr& hdr, char blockName[]);
    static void logAtomTapeHdr(ostream* fout, AtomTapeBlockHdr& hdr, char blockName[]);

    static void logBBMTapeHdr(BBMTapeBlockHdr& hdr, char blockName[]);
    static void logBBMTapeHdr(ostream* fout, BBMTapeBlockHdr& hdr, char blockName[]);


    static void logTAPFileHdr(TapeFile& tapeFile);
    static void logTAPFileHdr(ostream* fout, TapeFile& tapeFile);
    static void logTAPBlockHdr(FileBlock& block, uint32_t adr_offset, uint32_t blockNo = 0);
    static void logTAPBlockHdr(ostream* fout, FileBlock& block, uint32_t adr_offset, uint32_t blockNo = 0);



    static void copybytes(Byte* from, Byte* to, int n);
    static void copybytes(Byte* from, Bytes& to, int n);
    static void initbytes(Byte* bytes, Byte v, int n);
    static void initbytes(Bytes& bytes, Byte v, int n);

    static uint32_t bytes2uint(Byte* bytes, int n, bool littleEndian);
    static void uint2bytes(uint32_t u, Byte* bytes, int n, bool littleEndian);

    // Read file name from tape block
    static bool readTapeFileName(TargetMachine block_type, BytesIter& data_iter, Bytes& data, Word& CRC, char* name);


    // CRC calculation
    static void updateCRC(TargetMachine block_type, Word& CRC, Byte data);


    static string crDefaultOutFileName(string filePath);
    static string crDefaultOutFileName(string filePath, string fileExt);
    static string crEncodedFileNamefromDir(string dirPath, TapeFile tapeFile, string fileExt);

    // Log continuous memory data starting from an assumed address
    static void logData(int address, BytesIter& data_iter, int data_sz);
    static void logData(int address, Byte* data, int sz);

    

    static double decodeTime(string time);
    static string encodeTime(double time);

    static string blockNameFromFilename(TargetMachine block_type, string filename);


    static string filenameFromBlockName(string blockName);

    static bool decodeFileBlockHdr(
        TargetMachine block_type,
        FileBlock block, int nReadChars,
        int& loadAdr, int& execAdr, int& blockSz,
        bool& isBasicProgram, string& fileName
    );

    static bool decodeAtomTapeHdr(AtomTapeBlockHdr& hdr, FileBlock& block,
        int& loadAdr, int& execAdr, int& blockLen, int& blockNo, BlockType& blockType
    );

    static bool decodeBBMTapeHdr(BBMTapeBlockHdr& hdr, FileBlock& block,
        uint32_t& loadAdr, uint32_t& execAdr, uint32_t& blockLen, int& blockNo, uint32_t& nextAdr, bool& isLastBlock
    );

    static bool encodeTapeHdr(TargetMachine block_type, FileBlock& block, int block_no, int n_blocks, Bytes& hdr);

    static bool initTapeHdr(FileBlock& block, CapturedBlockTiming& block_info);
    static bool initTapeHdr(FileBlock& block);

private:

    static string roundedPD(string prefix, double d, string suffix);

    static char digitToHex(int d);

    static void updateAtomCRC(Word& CRC, Byte data);
    static void updateBBMCRC(Word& CRC, Byte data);

    static BlockType parseAtomTapeBlockFlag(AtomTapeBlockHdr hdr, int& blockLen);

    static string atomBlockNameFromFilename(string filename);
    static string bbmBlockNameFromFilename(string fn);

    static bool decodeATMBlockHdr(
        FileBlock block, int nReadChars,
        int& loadAdr, int& execAdr, int& blockSz,
        bool& isBasicProgram, string& fileName
    );

    static bool decodeBTMBlockHdr(
        FileBlock block, int nReadChars,
        int& loadAdr, int& execAdr, int& blockSz,
        bool& isBasicProgram, string& fileName
    );

    static bool readAtomTapeFileName(BytesIter& data_iter, Bytes& data, Word& CRC, char* name);
    static bool readBBMTapeFileName(BytesIter& data_iter, Bytes& data, Word& CRC, char* name);

    static bool encodeAtomTapeHdr(FileBlock& block, int block_no, int n_blocks, Bytes& hdr);
    static bool encodeBBMTapeHdr(FileBlock& block, Bytes& hdr);

};

#endif


