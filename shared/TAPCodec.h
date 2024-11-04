#pragma once

#ifndef TAP_CODEC_H
#define TAP_CODEC_H

#include <vector>
#include <string>
#include <cstdint>
#include "CommonTypes.h"
#include "FileBlock.h"
#include "Logging.h"

using namespace std;

//
//	ATM File structure
//	
//	 Conforms to the ATM header format specified by Wouter Ras.
//
//	Used by the AtomTapeUtils to store both Acorn Atom and BBC Micro programs
//
#define ATM_HDR_NAM_SZ 16
typedef struct ATMHdr_struct {
	char name[ATM_HDR_NAM_SZ]; // zero-padded with '\0' at the end
	Byte loadAdrLow; //
	Byte loadAdrHigh; // Load address (normally 0x2900 for BASIC programs)
	Byte execAdrLow; //
	Byte execAdrHigh; // Execution address (normally 0xc2b2 for BASIC programs)
	Byte lenLow;
	Byte lenHigh; // Length in bytes of the data section (normally data is as BASIC program)
} ATMHdr;

class TAPCodec
{

	//
	// Codec for the TAP Format
	// 
	// The Format is the one introduced by Wouter Ras for his Atom Emulator 1997 [https://www.stairwaytohell.com/atom/wouterras/]
	// Although originally only intended for the Acorn Atom it works as well for BBC Micro machines (Electron, BBC Micro, BBC Master)
	// and it therefor by the AtomUtils used as a supported for the latter machines.
	//
	// Format is <program name:16> <exec adr:2> <load adr:2> <file sz:2> <program data>
	// 
	// The execution & load addresses as well as the file size have big-endian encoding,
	// i.e., the high byte comes first.
	// 
	// The program name is up to 12* characters and is zero-padded from the right.
	// 
	// For BBC Micro machines the program name can only be up to 10 characters (for compatibility with the BBC Micro
	// tape format (see https://beebwiki.mdfs.net/Acorn_cassette_format) whereas it can be up to 12 characters for the
	// Acorn Atom.
	//


public:

	/*
	 * Create TAP Codec.
	 */
	TAPCodec(Logging logging, TargetMachine targetMachine = ACORN_ATOM);

	bool openTapeFile(string& filePath);

	bool closeTapeFile();

	/*
	 * Encode TAP File structure as TAP file
	 */
	bool encode(TapeFile& tapFile, string & filePath);

	/*
	 * Encode TAP File structure into already open TAP file
	 */
	bool encode(TapeFile& tapFile);

	/*
	 * Decode TAP file as TAP File structure
	 */
	bool decode(string &dataFileName, TapeFile& tapFile);

	bool decodeSingleFile(ifstream& fin, streamsize file_size, TapeFile& tapFile);

	bool decodeMultipleFiles(string& tapFileName, vector<TapeFile> &atomFiles);

private:

	TargetMachine mTargetMachine = ACORN_ATOM;

	Logging mDebugInfo;

	// Information used when encoding multiples programs into one big TAP file
	vector<TapeFile> mTAPFiles;
	ofstream* mTapeFile_p = NULL;
};



#endif

