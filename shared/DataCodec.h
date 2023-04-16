#pragma once

#ifndef DATA_CODEC_H
#define DATA_CODEC_H

#include <string>
#include "BlockTypes.h"

class DataCodec
{

public:
	/*
	 * Create DATA Codec and initialise it with a TAP
	 * file structure. If the file is not complete,
	 * then 'complete' shall be set to false.
	 */
	DataCodec(TAPFile& tapFile);

	DataCodec();

	/*
	 * Encode TAP File structure as DATA file
	 */
	bool encode(string& filePath);

	// Used by decode() below as a first decode step
	bool decode2Bytes(string& dataFileName, int& startAdress, Bytes& data);

	/*
	 * Decode DATA file as TAP File structure
	 */
	bool decode(string& tapFileName);

	/*
	 * Get the codec's TAP file
	 */
	bool getTAPFile(TAPFile& tapFile);

	/*
	 * Reinitialise codec with a new TAP file
	 */
	bool setTAPFile(TAPFile& tapFile);

private:

	TAPFile mTapFile;
};

#endif

