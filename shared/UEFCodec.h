#pragma once
#ifndef UEF_CODEC_H
#define UEF_CODEC_H


#include <vector>
#include <string>
#include <cstdint>
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


// Data encoding as can be specified for a UEF complex data block
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

	// The info below is kept mainly for unsupported chunks
	uint32_t chunkSz;
	uint16_t chunkId;

	ChunkInfo(ChunkInfoType chunkType, double time1, int time2, double t) : chunkInfoType(chunkType), data1_fp(time1),
		data2_i(time2) {}
	ChunkInfo() {}

	void init() {
		chunkInfoType = ChunkInfoType::UNKNOWN; data1_fp = -1.0;  data2_i = -1; data.clear(); dataEncoding.init();
	}
};

class UEFCodec
{
	

private:


	//
	// UEF header and chunk types
	//

	enum CHUNK_ID {
		ORIGIN_CHUNK = 0x0000, TARGET_CHUNK = 0x0005,
		IMPLICIT_DATA_BLOCK_CHUNK = 0x100, DEFINED_TAPE_FORMAT_DATA_BLOCK_CHUNK = 0x0104,
		CARRIER_TONE_CHUNK = 0x0110, CARRIER_TONE_WITH_DUMMY_BYTE = 0x0111,
		INTEGER_GAP_CHUNK = 0x0112,
		BASE_FREQ_CHUNK = 0x113, SECURITY_CHUNK = 0x0114, PHASE_CHUNK = 0x0115, FP_GAP_CHUNK = 0x0116, 
		DATA_ENCODING_FORMAT_CHANGE_CHUNK = 0x117,
		UNKNOWN_CHUNK = 0xffff,
		// Currently unsupported general chunks:
		MANUAL_CHUNK = 0x0001,
		INLAY_SCAN_CHUNK = 0x0003,
		BIT_MULTIPLEXING_INFO_CHUNK = 0x0006,
		EXTRA_PALETTE_CHUNK = 0x0007,
		ROM_HINT_CHUNK = 0x0008,
		SHORT_TITLE_CHUNK = 0x0009,
		VISIBLE_AREA_CHUNK = 0x000a,
		// Currently unsupported tape chunks:
		MULTIPLEXED_DATA_BLOCK_CHUNK = 0x0101,
		EXPLICIT_TAPE_DATA_BLOCK_CHUNK = 0x0102,
		POSITION_MARKER = 0x0120,
		TAPE_SET_INFO = 0x0130,
		START_OF_TAPE_SIDE = 0x0131,

	} ;
#define CHUNK_ID_BYTES(x) {x & 0xff, (x >> 8) & 0xff}
#define _CHUNK_ID(x) \
	(x==ORIGIN_CHUNK?"ORIGIN_CHUNK":\
	(x==IMPLICIT_DATA_BLOCK_CHUNK?"IMPLICIT_DATA_BLOCK_CHUNK":\
	(x==DEFINED_TAPE_FORMAT_DATA_BLOCK_CHUNK?"DEFINED_TAPE_FORMAT_DATA_BLOCK_CHUNK":\
	(x==CARRIER_TONE_CHUNK?"CARRIER_TONE_CHUNK":\
	(x==PHASE_CHUNK?"PHASE_CHUNK":\
	(x==DATA_ENCODING_FORMAT_CHANGE_CHUNK?"DATA_ENCODING_FORMAT_CHANGE_CHUNK":\
	(x==SECURITY_CHUNK?"SECURITY_CHUNK":\
	(x==INTEGER_GAP_CHUNK?"INTEGER_GAP_CHUNK":\
	(x==FP_GAP_CHUNK?"FP_GAP_CHUNK":\
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
	typedef struct IntegerGapChunk_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(INTEGER_GAP_CHUNK), {2, 0, 0, 0}};
		Byte gap[2] = { 32, 3 }; // A value of n indicates a gap of n/(2*base frequency) seconds
	} IntegerGapChunk;

	// UEF gap chunk type 116
	typedef struct FPGapChunk_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(FP_GAP_CHUNK), {4, 0, 0, 0}};
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
	typedef struct DataEncodingFormatChangeChunk_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(DATA_ENCODING_FORMAT_CHANGE_CHUNK), {2, 0, 0, 0}};
		Byte baudRate[2]; // Baudrate, .e.g., 300 = {44, 1} <=> 1 * 256 + 44 = 300
	} DataEncodingFormatChangeChunk;

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
	typedef struct DefinedTapeFormatDBChunkHdr_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(DEFINED_TAPE_FORMAT_DATA_BLOCK_CHUNK), {0, 0, 0, 0}}; // chunk size = #packets + 3
		Byte bitsPerPacket = 8; // (8 for Acorn Atom)
		Byte parity = 'N'; // 'N', E' or 'O' <=> None, Even or Odd ('N' for Acorn Atom)
		Byte stopBitInfo = -1; // > 0 => #stop bits < 0 >= #stop bits with extra short wave cycle (-1 for Acorn Atom)

	} DefinedTapeFormatDBChunkHdr;

	// UEF data chunk of type 0100
	typedef struct ImplicitDataChunkHdr_struct {
		ChunkHdr chunkHdr = { CHUNK_ID_BYTES(IMPLICIT_DATA_BLOCK_CHUNK), {0, 0, 0, 0}};
	} ImplicitDataBlockChunkHdr;

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
	// Methods used when creating a UEF File
	//

	// Methods to write header and different types of chunks
	bool writeUEFHeader(ogzstream &fout, Byte majorVersion, Byte minorVersion);
	bool writeBaseFrequencyChunk(ogzstream&fout);
	bool writeFloatPrecGapChunk(ogzstream&fout, double duration);
	bool writeIntPrecGapChunk(ogzstream& fout, double duration);
	bool writeSecurityCyclesChunk(ogzstream&fout, int nCycles, Byte firstPulse, Byte lastPulse, Bytes cycles);
	string decode_security_cycles(SecurityCyclesChunkHdr& hdr, Bytes cycles);
	double security_cycles_length(string cycles);
	bool writeBaudrateChunk(ogzstream&fout);
	bool writePhaseChunk(ogzstream&fout);
	bool writeCarrierChunk(ogzstream&fout, double duration);
	bool writeCarrierChunkwithDummyByte(ogzstream& fout, int firstCycles, int followingCycles);
	bool writeComplexDataChunk(ogzstream&fout, Byte bitsPerPacket, Byte parity, Byte stopBitInfo, Bytes data, Word &CRC);
	bool writeSimpleDataChunk(ogzstream&fout, Bytes data, Word& CRC);
	bool writeOriginChunk(ogzstream&fout, string origin);
	bool writeTargetChunk(ogzstream& fout);

	bool addBytes2Vector(Bytes& v, Byte bytes[], int n);

	bool encodeAtom(TapeFile& tapeFile);
	bool encodeBBM(TapeFile& tapeFile);

	static bool encodeFloat(double val, Byte encoded_val[4]);


	//
	// Methods used when reading UEF File
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

	Logging mDebugInfo;

	TargetMachine mTargetMachine = ACORN_ATOM;


	//
	// Data kept wile both reading and writiing a UEF file
	// 

	double mBaseFrequency = 1200; // Default for a UEF file
	unsigned mBaudRate = 1200; // Default for a UEF file
	unsigned mPhase = 180; // Default for a UEF file

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

	double mTime = 0.0; // Tape 'time' when reading or writing a UEF file

	vector <UEFChkPoint> checkpoints;

	ostream* mFout = &cout;

	ogzstream *mTapeFile_p = NULL;
	string mTapeFilePath = "";
	
public:

	bool openTapeFile(string& filePath);
	bool closeTapeFile();

	bool setStdOut(ostream* fout);

	bool processChunk(ChunkInfo& chunkInfo);

	UEFCodec(Logging logging, TargetMachine mTargetMachine);

	UEFCodec(bool useOriginalTiming, Logging logging, TargetMachine mTargetMachine);

	bool setTapeTiming(TapeProperties tapeTiming);

	/*
	 * Encode TAP File structure as UEF file 
	 */
	bool encode(TapeFile& tapFile, string& filePath);

	/*
	 * Encode TAP File structure into already open UEF file
	 */
	bool encode(TapeFile& tapeFile);

	/*
	 * Decode UEF file but only print it's content rather than converting a Tape File 
	 */
	bool inspect(string& uefFileName);

	double getTime();
	bool rollback();
	bool checkpoint();

	// Remove checkpoint (without rolling back)
	bool regretCheckpoint();

	int getPhaseShift();

	double getBaseFreq();

	bool readFromDataChunk(int n, Bytes& data);
	bool detectGap(double& duration1);
	bool detectCarrierWithDummyByte(double& waitingTime, double& duration1, double& duration2);
	bool detectCarrierWithDummyByte(double& duration1, double& duration2);
	bool detectCarrier(double& waitingTime, double& duration);
	bool detectCarrier(double& duration);

	bool readUefFile(string& uefFileName);
	bool validUefFile(string& uefFileName);
	
	
protected:

	bool writeFileIndepPart(TapeFile& tapeFile);

	bool mFirstFile = true;


};

#endif
