#pragma once
#ifndef UEF_CODEC_H
#define UEF_CODEC_H


#include <vector>
#include <string>
#include <gzstream.h>
#include "TAPCodec.h"
#include "../shared/TapeProperties.h"


using namespace std;



class UEFCodec
{

private:

	TAPFile mTapFile;

	TapeProperties mTapeTiming;

	


	//
	// UEF header and chunks
	//

	// UEF Header
	typedef struct UefHdr_struct {
		char eufTag[10] = "UEF File!"; // null-terminaed by compiler!
		Byte eufVer[2] = { 10, 0 };
	} UefHdr;

	// UEF chunk header
	typedef Byte ChunkId[2];
	typedef Byte ChunkSz[4];
	typedef struct {
		ChunkId chunkId;
		ChunkSz chunkSz;
	} ChunkHdr;

	// UEF base frequency chunk
	typedef struct BaseFreqChangeChunk0113_struct {
		ChunkHdr chunkHdr = { {0x13,0x1}, {4, 0, 0, 0} };
		Byte frequency[4]; // 32-bit IEEE 754 number representing the base frequency (default is 1200 Hz)
	} BaseFreqChangeChunk0113;

	// UEF gap chunk type 112
	typedef struct BaudwiseGapChunk0112_struct {
		ChunkHdr chunkHdr = { {0x12,0x1}, {2, 0, 0, 0} };
		Byte gap[2] = { 32, 3 }; // in (1 / (baud rate * 2))ths of a second : 800 / (300 *2) = 1.3 s
	} BaudwiseGapChunk0112;

	// UEF gap chunk type 116
	typedef struct FPGap0116_struct {
		ChunkHdr chunkHdr = { {0x16,0x1}, {4, 0, 0, 0} };
		Byte gap[4]; // 32-bit IEEE 754 number representing gap in seconds
	} FPGap0116;

	//
	// UEF security cycles chunk
	// 
	// The UEF is encoded with eight 'cycles' per byte.
	// Slow cycles(at the base frequency) are denoted by 0 bits.
	// Fast cycles(at twice the base frequency) are denoted by 1 bits.
	// Bits are ordered such that the most significant bit represents the first cycle on the tape.
	// Spare bits in the last byte should preferably be 0 bits.
	// #cycle bytes = ceil(#cycles / 8); chunk size = 5 + #cycle bytes
	typedef struct SecurityCycles0114Hdr_struct {
		ChunkHdr chunkHdr = { {0x14,0x1}, {0, 0, 0, 0} }; // size
		Byte nCycles[3]; // 
		Byte firstPulse; // 'P' or 'W'; 'P' => the first cycle is replaced by a single high pulse.
		Byte lastPulse; // 'P' or 'W'; 'P' => the last  cycle is replaced by a single low pulse (and then firstPluse bust be 'W')
	} SecurityCycles0114Hdr;

	// UEF baudrate chunk
	typedef struct DataFormatChange0117_struct {
		ChunkHdr chunkHdr = { {0x17,0x1}, {2, 0, 0, 0} };
		Byte baudRate[2]; // Baudrate, .e.g., 300 = {44, 1} <=> 1 * 256 + 44 = 300
	} DataFormatChange0117;

	// UEF phase chunk
	typedef struct PhaseChangeChunk0115_struct {
		ChunkHdr chunkHdr = { {0x15,0x1}, {2, 0, 0, 0} };
		Byte phase[2] = { 0, 0 }; // zero degrees phase shift
	} PhaseChangeChunk0115;

	
	// UEF high tone chunk
	typedef struct HighToneChunk0110_struct {
		ChunkHdr chunkHdr = { {0x10,0x1}, {2, 0, 0, 0} };
		Byte duration[2] = { 224, 11 }; //  in (1/(baud rate*2))ths of a second: 3060 / (300 * 2) =  5.1 s
	} HighToneChunk0110;

	//
	// UEF data chunk of type 0104
	// 
	// Data blocks are always stored in the chunk as whole byte quantities.
	// If the number of data bits is seven then the most significant bits
	// of all bytes in the chunk are unused and should be zero
	typedef struct DataBlockChunk0104Hdr_struct {
		ChunkHdr chunkHdr = { { 0x4, 0x1}, {0, 0, 0, 0} }; // chunk size = #packets + 3
		Byte bitsPerPacket = 8; // (8 for Acorn Atom)
		Byte parity = 'N'; // 'N', E' or 'O' <=> None, Even or Odd ('N' for Acorn Atom)
		Byte stopBitInfo = -1; // > 0 => #stop bits < 0 >= #stop bits with extra short wave cycle (-1 for Acorn Atom)

	} DataBlockChunk0104Hdr;

	// UEF data chunk of type 0100
	typedef struct DataBlockChunk0100Hdr_struct {
		ChunkHdr chunkHdr = {0x0,0x1, {0, 0, 0, 0} };
	} DataBlockChunk0100Hdr;

	// UEF origin chunk
	typedef struct OriginChunkHdr0000_struct {
		ChunkHdr chunkHdr = { {0,0}, {0, 0, 0, 0} };
	} OriginChunkHdr0000;


	// Methods to write header and different types of chunks
	static bool writeUEFHeader(ogzstream &fout, Byte majorVersion, Byte minorVersion);
	static bool writeBaseFrequencyChunk(ogzstream&fout, float baseFrequency);
	static bool writeFloatPrecGapChunk(ogzstream&fout, float duration);
	static bool writeIntPrecGapChunk(ogzstream& fout, float duration, int baudRate);
	static bool writeSecurityCyclesChunk(ogzstream&fout, int nCycles, Byte firstPulse, Byte lastPulse, Bytes cycles);
	static string decode_security_cycles(SecurityCycles0114Hdr& hdr, Bytes cycles);
	static bool writeBaudrateChunk(ogzstream&fout, int baudrate);
	static bool writePhaseChunk(ogzstream&fout, int phase);
	static bool writeHighToneChunk(ogzstream&fout, float duration, int baudRate);
	static bool writeData104Chunk(ogzstream&fout, Byte bitsPerPacket, Byte parity, Byte stopBitInfo, Bytes data, Byte &CRC);
	static bool writeData100Chunk(ogzstream&fout, Bytes data, Byte & CRC);
	static bool writeOriginChunk(ogzstream&fout, string origin);

	static bool addBytes2Vector(Bytes& v, Byte bytes[], int n);

public:


	UEFCodec();

	static bool decodeFloat(Byte encoded_val[4], float& decoded_val);

	static bool encodeFloat(float val, Byte encoded_val[4]);

	UEFCodec(TAPFile& tapFile);

	UEFCodec(string& abcFileName);

	bool setTapeTiming(TapeProperties tapeTiming);
	/*
	 * Encode TAP File structure as UEF file 
	 */
	bool encode(string& filePath);

	/*
	 * Decode UEF file as TAP File structure
	 */
	bool decode(string &uefFileName);

	/*
	 * Get the codec's TAP file
	 */
	bool getTAPFile(TAPFile& tapFile);

	/*
	 * Reinitialise codec with a new TAP file
	 */
	bool setTAPFile(TAPFile& tapFile);



private:

	static bool read_bytes(BytesIter& data_iter, Bytes& data, int n, Byte& CRC, Bytes& block_data);

	static bool read_bytes(BytesIter& data_iter, Bytes& data, int n, Byte& CRC, Byte bytes[]);

	static bool check_bytes(BytesIter& data_iter, Bytes& data, int n, Byte& CRC, Byte refVal);

	static bool read_block_name(BytesIter& data_iter, Bytes& data, Byte& CRC, char name[13]);

};

#endif