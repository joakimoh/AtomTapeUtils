#pragma once
#ifndef UEF_CODEC_H
#define UEF_CODEC_H


#include <vector>
#include <string>
#include <gzstream.h>
#include "TAPCodec.h"
#include "../shared/TapeProperties.h"
#include "../shared/TapeProperties.h"
#include "BitTiming.h"


using namespace std;

enum ChunkInfoType { BAUDRATE, BASE_FREQ, CARRIER, CARRIER_DUMMY, DATA, GAP, PHASE, IGNORE, UNKNOWN };


#define _CHUNK_INFO_TYPE(x) (\
	x==CARRIER?"CARRIER":\
	(x==CARRIER_DUMMY?"CARRIER_DUMMY":\
	(x==DATA?"DATA":(x==GAP?"GAP":\
	(x==PHASE?"PHASE":(x==IGNORE?"IGNORE":(x==BAUDRATE?"BAUDRATE":(x==BASE_FREQ?"BASEFREQ":"UNKNOWN"))))))))


// Data encoding as can be specified for an UEF complex data block
// Default values are used for simple data chunks
enum Parity { NO_PAR, ODD, EVEN };
#define _PARITY(x) (x==Parity::NO_PAR?"-":(x==Parity::ODD?"ODD":"EVEN"))
class DataEncoding {
public:
	int bitsPerPacket = 8; // 7 or 8
	Parity parity = Parity::NO_PAR; // NONE/ODD/EVEN
	int nStopBits = 1; // 1 or 2 normally
	bool extraShortWave = false; // For Acorn Atom

	DataEncoding() { init(); }

	DataEncoding(int nb, Parity p, int nsb, bool esw) {
		bitsPerPacket = nb;
		parity = p;
		nStopBits = nsb;
		extraShortWave = esw;
	}

	void init() { bitsPerPacket = 8; parity = Parity::NO_PAR; nStopBits = 1; extraShortWave = false; }

	void log() {
		cout << dec << bitsPerPacket;
		cout << (parity == NO_PAR ? "N" : (parity == ODD ? "O" : "E"));
		if (extraShortWave)
			cout << "-";
		cout << nStopBits;
		cout << "\n";
	}
};

const DataEncoding atomDefaultDataEncoding(8, Parity::NO_PAR, 1, true); // 8N-1
const DataEncoding bbmDefaultDataEncoding(8, Parity::NO_PAR, 1, false); // 8N1



class ChunkInfo {

		
public:


	ChunkInfoType chunkInfoType = ChunkInfoType::UNKNOWN;
	double data1_fp = 0.0;
	int data2_i = 0;
	Bytes data;
	DataEncoding dataEncoding;

	ChunkInfo(ChunkInfoType chunkType, double time1, int time2) : chunkInfoType(chunkType), data1_fp(time1), data2_i(time2) {}
	ChunkInfo() {}

	void init() { chunkInfoType = ChunkInfoType::UNKNOWN; data1_fp = -1.0;  data2_i = -1; data.clear(); dataEncoding.init(); }
};

class UEFCodec
{
	

private:


	//
	// UEF header and chunk types
	//

	enum CHUNK_ID {
		ORIGIN_CHUNK = 0x0000, SIMPLE_DATA_CHUNK = 0x100, COMPLEX_DATA_CHUNK = 0x0104,
		CARRIER_TONE_CHUNK = 0x0110, CARRIER_TONE_WITH_DUMMY_BYTE = 0x0111, PHASE_CHUNK = 0x0115, BAUD_RATE_CHUNK = 0x117, SECURITY_CHUNK = 0x0114,
		BAUDWISE_GAP_CHUNK = 0x0112, FLOAT_GAP_CHUNK = 0x0116, BASE_FREQ_CHUNK = 0x113,
		TARGET_CHUNK = 0x0005, UNKNOWN_CHUNK = 0xffff
	} ;
#define CHUNK_ID_BYTES(x) {x & 0xff, (x >> 8) & 0xff}
#define _CHUNK_ID(x) \
	(x==ORIGIN_CHUNK?"ORIGIN_CHUNK":\
	(x==SIMPLE_DATA_CHUNK?"SIMPLE_DATA_CHUNK":\
	(x==COMPLEX_DATA_CHUNK?"COMPLEX_DATA_CHUNK":\
	(x==CARRIER_TONE_CHUNK?"CARRIER_TONE_CHUNK":\
	(x==PHASE_CHUNK?"PHASE_CHUNK":\
	(x==BAUD_RATE_CHUNK?"BAUD_RATE_CHUNK":\
	(x==SECURITY_CHUNK?"SECURITY_CHUNK":\
	(x==BAUDWISE_GAP_CHUNK?"BAUDWISE_GAP_CHUNK":\
	(x==FLOAT_GAP_CHUNK?"FLOAT_GAP_CHUNK":\
	(x==BASE_FREQ_CHUNK?"BASE_FREQ_CHUNK":\
	(x==TARGET_CHUNK?"TARGET_CHUNK":\
	(x== CARRIER_TONE_WITH_DUMMY_BYTE?"CARRIER_TONE_WITH_DUMMY_BYTE":"???")\
		)))))))))))

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

	// UEF pahse shift chunk
	typedef struct PhaseChunk_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(PHASE_CHUNK), {2, 0, 0, 0}};
		Byte phase_shift[2] = { 0, 0 }; // zero degrees phase_shift
	} PhaseChunk;

	
	// UEF carrier chunk
	typedef struct CarrierChunk_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(CARRIER_TONE_CHUNK), {2, 0, 0, 0}};
		Byte duration[2] = { 224, 11 }; //  in (1/(baud rate*2))ths of a second: 3060 / (300 * 2) =  5.1 s
	} CarrierChunk;

	// UEF carrier chunk with dummy byte
	typedef struct CarrierChunkDummy_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(CARRIER_TONE_WITH_DUMMY_BYTE), {4, 0, 0, 0} };
		Byte duration[4] = { 4, 0, 0xba, 0x2f };	// first bytes: #cycles of carrier before dummy byte (default 4 cycles)
													// last two bytes: #cycles of carrier after dummy byte (default 12218 cycles <=> 5.1s)
													// in (1/(2*base frequency))ths of a second: 3060 / (300 * 2) =  5.1 s
	} CarrierChunkDummy;

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
		typedef struct TargetMachineChunk_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(TARGET_CHUNK), {1, 0, 0, 0} };
		Byte targetMachine;
	} TargetMachineChunk;




	//
	// Methods used when creating an EUF File
	//

	// Methods to write header and different types of chunks
	bool writeUEFHeader(ogzstream &fout, Byte majorVersion, Byte minorVersion);
	bool writeBaseFrequencyChunk(ogzstream&fout);
	bool writeFloatPrecGapChunk(ogzstream&fout, double duration);
	bool writeIntPrecGapChunk(ogzstream& fout, double duration);
	bool writeSecurityCyclesChunk(ogzstream&fout, int nCycles, Byte firstPulse, Byte lastPulse, Bytes cycles);
	string decode_security_cycles(SecurityCyclesChunkHdr& hdr, Bytes cycles);
	bool writeBaudrateChunk(ogzstream&fout);
	bool writePhaseChunk(ogzstream&fout);
	bool writeCarrierChunk(ogzstream&fout, double duration);
	bool writeCarrierChunkwithDummyByte(ogzstream& fout, int firstCycles, int followingCycles);
	bool writeComplexDataChunk(ogzstream&fout, Byte bitsPerPacket, Byte parity, Byte stopBitInfo, Bytes data, Word &CRC);
	bool writeSimpleDataChunk(ogzstream&fout, Bytes data, Word& CRC);
	bool writeOriginChunk(ogzstream&fout, string origin);
	bool writeTargetChunk(ogzstream& fout);

	bool addBytes2Vector(Bytes& v, Byte bytes[], int n);

	bool encodeAtom(TapeFile& tapeFile, ogzstream& fout);
	bool encodeBBM(TapeFile& tapeFile, ogzstream& fout);

	static bool encodeFloat(double val, Byte encoded_val[4]);


	//
	// Methods used when reading EUF File
	//

	static bool decodeFloat(Byte encoded_val[4], double& decoded_val);

	bool readBytes(Byte* dst, int n);

	bool detectCarrier(double& waitingTime, double& duration1, double& duration2, bool skipData, bool acceptDummy);

	

	void consume_bytes(int& n, Bytes& data);

	
	//
	// General properties used by the codec
	//

	TapeProperties mTapeTiming;

	bool mUseOriginalTiming = false;

	bool mVerbose = false;

	TargetMachine mTargetMachine = ACORN_ATOM;


	//
	// Data kept wile both reading and writiing an UEF file
	// 

	double mBaseFrequency = 1200; // Default for an UEF file
	unsigned mBaudRate = 1200; // Default for an UEF file
	unsigned mPhase = 180; // Default for an UEF file

	//
	// Data kept while writing UEF file
	//

	bool firstBlock = true;
	


	//
	// Data kept while reading UEF file
	//

	class UEFChkPoint {

	public:
		BytesIter pos;
		double time = 0;
		Bytes bufferedData;
		enum {};

		UEFChkPoint(BytesIter p, double t, Bytes data) : pos(p), time(t), bufferedData(data) {}
	};

	BitTiming mBitTiming;
	Bytes mRemainingData;
	Bytes mUefData;
	BytesIter mUefDataIter;
	double mTime = 0.0;
	vector <UEFChkPoint> checkpoints;

	ostream* mFout = &cout;
	
public:

	bool setStdOut(ostream* fout);

	bool processChunk(ChunkInfo& chunkInfo);

	UEFCodec(bool verbose, TargetMachine mTargetMachine);

	UEFCodec(bool useOriginalTiming, bool verbose, TargetMachine mTargetMachine);

	bool setTapeTiming(TapeProperties tapeTiming);

	/*
	 * Encode TAP File structure as UEF file 
	 */
	bool encode(TapeFile& tapFile, string& filePath);

	/*
	 * Decode UEF file but only print it's content rather than converting a Tape File 
	 */
	bool inspect(string& uefFileName);

	double getTime();
	bool rollback();
	bool checkpoint();

	int getPhaseShift();

	double getBaseFreq();

	bool readfromDataChunk(int n, Bytes& data);
	bool detectGap(double& duration1);
	bool detectCarrierWithDummyByte(double& waitingTime, double& duration1, double& duration2);
	bool detectCarrierWithDummyByte(double& duration1, double& duration2);
	bool detectCarrier(double& waitingTime, double& duration);
	bool detectCarrier(double& duration);

	bool readUefFile(string& uefFileName);
	bool validUefFile(string& uefFileName);
	
	



};

#endif
