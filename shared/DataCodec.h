#pragma once

#ifndef DATA_CODEC_H
#define DATA_CODEC_H

#include <string>
#include "BlockTypes.h"

class DataCodec
{

public:
	/*
	 * Create DATA Codec.
	 */
	DataCodec(bool verbose);

	/*
	 * Encode TAP File structure as DATA file
	 */
	bool encode(TapeFile &tapFile, string& filePath);

	// Used by decode() below as a first decode step
	bool decode2Bytes(string& dataFileName, int& startAdress, Bytes& data);

	/*
	 * Decode DATA file as TAP File structure
	 */
	bool decode(string& tapFileName, TapeFile &tapFile);

private:

	bool mVerbose = false;
};

#endif

