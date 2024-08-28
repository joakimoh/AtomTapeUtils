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
	bool DataCodec::encodeBBM(TapeFile& tapeFile, string& filePath, ofstream& fout);
	bool DataCodec::encodeAtom(TapeFile& tapeFile, string& filePath, ofstream& fout);
	
	/*
	 * Decode DATA file as TAP File structure
	 */
	bool decode(string& tapFileName, TapeFile &tapFile, bool bbcMicro);

	bool data2Bytes(string& dataFileName, int& startAdress, Bytes& data);

private:

	bool mVerbose = false;

	
};

#endif

