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


class TAPCodec
{

public:

	


	/*
	 * Create TAP Codec.
	 */
	TAPCodec(Logging logging);

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

	static bool tap2Bytes(TapeFile& tapeFile, uint32_t& loadAdress, Bytes& data);
	

	bool bytes2TAP(Bytes& data, TargetMachine targetMachine, string tapeFileNam, uint32_t loadAdr, uint32_t exceAdr, TapeFile& tapeFile);


	static bool data2Binary(TapeFile& tapFile, string& binFileName);

private:

	Logging mDebugInfo;

	// Information used when encoding multiples programs into one big TAP file
	vector<TapeFile> mTAPFiles;
	ofstream* mTapeFile_p = NULL;
};



#endif

