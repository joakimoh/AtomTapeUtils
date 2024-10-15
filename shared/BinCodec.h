#pragma once

#ifndef BIN_CODEC_H
#define BIN_CODEC_H

#include <string>
#include "FileBlock.h"
#include "Logging.h"



class BinCodec
{

public:

	//
	// Create BIN Codec.
	//
	BinCodec(Logging logging);

	//
	// Encode TAP File structure as binary data
	//
	bool encode(TapeFile& tapFile, FileMetaData &fileMetaData, Bytes &data);

	//
	// Decode binary data as TAP File structure
	//
	bool decode(FileMetaData fileMetaData, Bytes &data, TapeFile& tapFile);


private:

	Logging mDebugInfo;


};

#endif

