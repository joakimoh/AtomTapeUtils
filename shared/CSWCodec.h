#pragma once
#ifndef CSW_CODEC_H
#define CSW_CODEC_H


#include <vector>
#include <string>
#include <gzstream.h>
#include "TAPCodec.h"
#include "../shared/TapeProperties.h"
#include "../shared/WaveSampleTypes.h"
#include "../shared/BlockTypes.h"

using namespace std;





class CSWCodec
{ 
	// Default values used: 1f 8b 08 00 00 00
	typedef struct CompressedFileHdr_struct {
		Byte CmF = 0x1f;
		Byte Flg = 0x8b;
		Byte DictId[4] = { 0x08, 0x00, 0x00, 0x00 };
	} CompressedFileHdr;

	// Version-independent part of CSW header
	typedef struct CSWCommonHdr_struct {
		char signature[22]; // "Compressed Square Wave"
		Byte terminatorCode = 0x1a;
		Byte majorVersion;
		Byte minorVersion;
	} CSWCommonHdr;

	//
	// CSW2 format (v2.0) used by most recent tools
	//

	// CSW2-unique header part

	typedef struct CSW2MainHdr_struct {
		Byte sampleRate[4]; // sample rate (little-endian)
		Byte totNoPulses[4]; // #pulses (little-endian)
		Byte compType = 0x01; // 0x01 (RLE) - no compression; 0x02 (Z-RLE) - ZLIB compression ONLY of data stream
		Byte flags; // b0: initial polarity; if set, the signal starts at logical high
		Byte hdrExtLen; // For future expansions only
		char encodingApp[16] = { 'A','t','o','m','t','a','p','e','U','t','i','l','s'};
	} CSW2MainHdr;

	//
	// CSW Format
	//
	// Specifies pulses that corresponds to the duration (in # samples) of a LOW or HIGH half_cycle
	//
	// --  PULSE #1 --|-- PULSE #2 ---
	// (8 samples)    | (5 samples)
	// x x x x x x x x
	//                 x x x x x
	//

	// The complete CSW2 header
	typedef struct CSW2Hdr_struct {
		CSWCommonHdr common = { {'C','o','m','p','r','e','s','s','e','d',' ','S','q','u','a','r','e',' ','W','a','v','e'} , 0x1a, 0x02, 0x00 };
		CSW2MainHdr csw2;
		// Byte hdrExtData[hdrExtLen]; // For future expansions only
		// Byte data[totNoPulses];
	} CSW2Hdr;


	//
	// Legacy CSW format (v1.1) used by some tools
	//

	// CSW1-unique header part
	typedef struct CSW1MainHdr_struct {
		Byte sampleRate[2]; // sample rate (little-endian)
		Byte compType = 0x01; // 0x01 (RLE) - no compression; 0x02 (Z-RLE) - ZLIB compression ONLY of data stream
		Byte flags; // b0: initial polarity; if set, the signal starts at logical high
		Byte reserved[3]; // Reserved
	} CSW1MainHdr;

	// The complete CSW1 header
	typedef struct CSW1Hdr_struct {
		CSWCommonHdr common = { {'C','o','m','p','r','e','s','s','e','d',' ','S','q','u','a','r','e',' ','W','a','v','e'} , 0x1a, 0x01, 0x01 };
		CSW1MainHdr csw1;
		// Byte data[];

	} CSW1Hdr;

private:

	TapeProperties mTapeTiming;

	bool mUseOriginalTiming = false;

	bool mVerbose = false;
	bool mBbcMicro = false;

public:

	// Default constructor
	CSWCodec(bool verbose, bool bbcMicro);

	// Constructor
	CSWCodec(bool useOriginalTiming, bool verbose, bool bbcMicro);

	// Reinitialise codec with a new tape timing
	bool setTapeTiming(TapeProperties tapeTiming);

	// Encode a TAP File structure as CSW file
	bool encode(TapeFile &tapeFile, string& filePath, int sampleFreq);

	 // Decode a CSW file as a vector of pulses
	bool decode(string &CSWFileName, Bytes &pulses, int &sampleFreq, HalfCycle &firstHalfCycle);

	// Tell whether a file is a CSW file
	static bool isCSWFile(string& CSWFileName);


private:

	// Pulse data
	int mPulseIndex;
	HalfCycle mPulseLevel;
	int mSampleIndex;
	int mPulseLength;

	// Pulses read or to write
	Bytes mPulses;

	Byte mCRC = 0;
	int mFS = 44100;
	int mPhase = 180;

	double mHighSamples;
	double mLowSamples;;

	int mStartBitCycles;	// #cycles for start bit - should be 1 "bit"
	int mLowDataBitCycles; // #cycles for LOW "0" data bit - for F2 frequency
	int mHighDataBitCycles;	//#cycles for HIGH "1" data bit - for F1 frequency
	int mStopBitCycles;		// #cycles for stop bit - should be 1 1/2 "bit"

	bool writeByte(Byte byte);
	bool writeDataBit(int bit);
	bool writeStartBit();
	bool writeStopBit();
	bool writeCycle(bool high, unsigned n);
	bool writeTone(double duration);
	bool writeGap(double duration);

};

#endif