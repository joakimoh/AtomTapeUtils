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

	bool mUseOriginalTiming = false;

	bool mVerbose = false;


	//
	// UEF header and chunks
	//

	typedef enum {
		ORIGIN_CHUNK = 0x0000, SIMPLE_DATA_CHUNK = 0x100, COMPLEX_DATA_CHUNK = 0x0104,
		HIGH_TONE_CHUNK = 0x0110, PHASE_CHUNK = 0x0115, BAUD_RATE_CHUNK = 0x117, SECURITY_CHUNK = 0x0114,
		BAUDWISE_GAP_CHUNK = 0x0112, FLOAT_GAP_CHUNK = 0x0116, BASE_FREQ_CHUNK = 0x113,
		TARGET_CHUNK = 0x0005, UNKNOWN_CHUNK = 0xffff
	} CHUNK_ID;
#define CHUNK_ID_BYTES(x) {x & 0xff, (x >> 8) & 0xff}
#define _CHUNK_ID(x) \
	(x==ORIGIN_CHUNK?"ORIGIN_CHUNK":\
	(x==SIMPLE_DATA_CHUNK?"SIMPLE_DATA_CHUNK":\
	(x==COMPLEX_DATA_CHUNK?"COMPLEX_DATA_CHUNK":\
	(x==HIGH_TONE_CHUNK?"HIGH_TONE_CHUNK":\
	(x==PHASE_CHUNK?"PHASE_CHUNK":\
	(x==BAUD_RATE_CHUNK?"BAUD_RATE_CHUNK":\
	(x==SECURITY_CHUNK?"SECURITY_CHUNK":\
	(x==BAUDWISE_GAP_CHUNK?"BAUDWISE_GAP_CHUNK":\
	(x==FLOAT_GAP_CHUNK?"FLOAT_GAP_CHUNK":\
	(x==BASE_FREQ_CHUNK?"BASE_FREQ_CHUNK":\
	(x==TARGET_CHUNK?"TARGET_CHUNK":"???")))))))))))

	// UEF Header
	typedef struct UefHdr_struct {
		char uefTag[10] = "UEF File!"; // null-terminated by compiler!
		Byte uefVer[2] = { 10, 0 };
	} UefHdr;

	// UEF chunk header
	typedef Byte ChunkId[2];
	typedef Byte ChunkSz[4];
	typedef struct {
		ChunkId chunkId;
		ChunkSz chunkSz;
	} ChunkHdr;



	// UEF base frequency chunk
	typedef struct BaseFreqChunk_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(BASE_FREQ_CHUNK), {4, 0, 0, 0} };
		Byte frequency[4]; // 32-bit IEEE 754 number representing the base frequency (default is 1200 Hz)
	} BaseFreqChunk;

	// UEF gap chunk type 112
	typedef struct BaudwiseGapChunk_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(BAUDWISE_GAP_CHUNK), {2, 0, 0, 0}};
		Byte gap[2] = { 32, 3 }; // in (1 / (baud rate * 2))ths of a second : 800 / (300 *2) = 1.3 s
	} BaudwiseGapChunk;

	// UEF gap chunk type 116
	typedef struct FPGapChunk_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(FLOAT_GAP_CHUNK), {4, 0, 0, 0}};
		Byte gap[4]; // 32-bit IEEE 754 number representing gap in seconds
	} FPGapChunk;

	//
	// UEF security cycles chunk
	// 
	// The UEF is encoded with eight 'cycles' per byte.
	// Slow cycles(at the base frequency) are denoted by 0 bits.
	// Fast cycles(at twice the base frequency) are denoted by 1 bits.
	// Bits are ordered such that the most significant bit represents the first cycle on the tape.
	// Spare bits in the last byte should preferably be 0 bits.
	// #cycle bytes = ceil(#cycles / 8); chunk size = 5 + #cycle bytes
	typedef struct SecurityCyclesChunkHdr_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(SECURITY_CHUNK), {0, 0, 0, 0}}; // size
		Byte nCycles[3]; // 
		Byte firstPulse; // 'P' or 'W'; 'P' => the first cycle is replaced by a single high pulse.
		Byte lastPulse; // 'P' or 'W'; 'P' => the last  cycle is replaced by a single low pulse (and then firstPluse bust be 'W')
	} SecurityCyclesChunkHdr;

	// UEF baudrate chunk
	typedef struct BaudRateChunk_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(BAUD_RATE_CHUNK), {2, 0, 0, 0}};
		Byte baudRate[2]; // Baudrate, .e.g., 300 = {44, 1} <=> 1 * 256 + 44 = 300
	} BaudRateChunk;

	// UEF half_cycle chunk
	typedef struct PhaseChunk_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(PHASE_CHUNK), {2, 0, 0, 0}};
		Byte half_cycle[2] = { 0, 0 }; // zero degrees phase_shift
	} PhaseChunk;

	
	// UEF high tone chunk
	typedef struct HighToneChunk_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(HIGH_TONE_CHUNK), {2, 0, 0, 0}};
		Byte duration[2] = { 224, 11 }; //  in (1/(baud rate*2))ths of a second: 3060 / (300 * 2) =  5.1 s
	} HighToneChunk;

	//
	// UEF data chunk of type 0104
	// 
	// Data blocks are always stored in the chunk as whole byte quantities.
	// If the number of data bits is seven then the most significant bits
	// of all bytes in the chunk are unused and should be zero
	typedef struct ComplexDataBlockChunkHdr_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(COMPLEX_DATA_CHUNK), {0, 0, 0, 0}}; // chunk size = #packets + 3
		Byte bitsPerPacket = 8; // (8 for Acorn Atom)
		Byte parity = 'N'; // 'N', E' or 'O' <=> None, Even or Odd ('N' for Acorn Atom)
		Byte stopBitInfo = -1; // > 0 => #stop bits < 0 >= #stop bits with extra short wave cycle (-1 for Acorn Atom)

	} ComplexDataBlockChunkHdr;

	// UEF data chunk of type 0100
	typedef struct SimpleDataBlockChunkHdr_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(SIMPLE_DATA_CHUNK), {0, 0, 0, 0}};
	} SimpleDataBlockChunkHdr;

	// UEF origin chunk
	typedef struct OriginChunk_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(ORIGIN_CHUNK), {0, 0, 0, 0}};
	} OriginChunk;

	// UEF Target machine chunk of type 0005
	typedef enum TargetMachine {BBC_MODEL_A = 0, ELECTRON = 1, BBC_MODEL_B = 2, BBC_MASTER = 3, ATOM = 4, UNKNOWN = 0xff};
#define _TARGET_MACHINE(x) (x==BBC_MODEL_A?"BBC_MODEL_A": \
	(x==ELECTRON?"ELECTRON":(x==BBC_MODEL_B?"BBC_MODEL_B":(x==BBC_MASTER?"BBC_MASTER":(x==ATOM?"ATOM":"???")))))
	typedef struct TargetMachineChunk_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(TARGET_CHUNK), {1, 0, 0, 0} };
		Byte targetMachine;
	} TargetMachineChunk;



	float mBaseFrequency = 1200;
	unsigned mBaudRate = 300;
	unsigned mPhase = 180;
	TargetMachine mTarget = ATOM;


	// Methods to write header and different types of chunks
	bool writeUEFHeader(ogzstream &fout, Byte majorVersion, Byte minorVersion);
	bool writeBaseFrequencyChunk(ogzstream&fout);
	bool writeFloatPrecGapChunk(ogzstream&fout, float duration);
	bool writeIntPrecGapChunk(ogzstream& fout, float duration);
	bool writeSecurityCyclesChunk(ogzstream&fout, int nCycles, Byte firstPulse, Byte lastPulse, Bytes cycles);
	string decode_security_cycles(SecurityCyclesChunkHdr& hdr, Bytes cycles);
	bool writeBaudrateChunk(ogzstream&fout);
	bool writePhaseChunk(ogzstream&fout);
	bool writeHighToneChunk(ogzstream&fout, float duration);
	bool writeComplexDataChunk(ogzstream&fout, Byte bitsPerPacket, Byte parity, Byte stopBitInfo, Bytes data, Byte &CRC);
	bool writeSimpleDataChunk(ogzstream&fout, Bytes data, Byte & CRC);
	bool writeOriginChunk(ogzstream&fout, string origin);
	bool writeTargetChunk(ogzstream& fout);


	//
	// Data kept while parsing UEF file
	//

	typedef enum {
		READING_GAP, READING_LEAD_TONE, READING_HDR, READING_MICRO_TONE, READING_DATA,UNDEFINED_BLOCK_STATE
	} BLOCK_STATE;

	typedef enum { TONE_CHUNK, DATA_CHUNK, GAP_CHUNK, UNDEFINED_CHUNK } CHUNK_TYPE;

#define _BLOCK_STATE(x) \
    (x==READING_GAP?"READING_GAP":\
    (x==READING_LEAD_TONE?"READING_LEAD_TONE":(x==READING_HDR?"READING_HDR":\
    (x==READING_MICRO_TONE?"READING_MICRO_TONE":(x==READING_DATA?"READING_DATA":"???")))))

	typedef struct BLOCK_INFO_struct {
		int lead_tone_cycles;
		int micro_tone_cycles;
		double block_gap;
		int half_cycle = 180;
		double start;
		double end;
	} BLOCK_INFO;
	BLOCK_STATE mBlockState = READING_GAP;
	BLOCK_STATE mPrevBlockState = UNDEFINED_BLOCK_STATE;
	vector<BLOCK_INFO> mBlockInfo;
	BLOCK_INFO mCurrentBlockInfo;
	float mCurrentTime = 0;
	bool firstBlock = true;
	bool updateBlockState(CHUNK_TYPE chunkType, double duration);



	bool addBytes2Vector(Bytes& v, Byte bytes[], int n);

	// Methods to decode bytes
	static bool read_bytes(BytesIter& data_iter, Bytes& data, int n, Byte& CRC, Bytes& block_data);
	static bool read_bytes(BytesIter& data_iter, Bytes& data, int n, Byte& CRC, Byte bytes[]);
	static bool check_bytes(BytesIter& data_iter, Bytes& data, int n, Byte& CRC, Byte refVal);
	static bool read_block_name(BytesIter& data_iter, Bytes& data, Byte& CRC, char name[ATM_MMC_HDR_NAM_SZ]);


public:


	UEFCodec(bool verbose);

	static bool decodeFloat(Byte encoded_val[4], float& decoded_val);

	static bool encodeFloat(float val, Byte encoded_val[4]);

	UEFCodec(TAPFile& tapFile, bool useOriginalTiming, bool verbose);

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




};

#endif