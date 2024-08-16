#pragma once

#ifndef TAP_CODEC_H
#define TAP_CODEC_H

#include <vector>
#include <string>
#include "CommonTypes.h"
#include "BlockTypes.h"

using namespace std;


class TAPCodec
{

public:

	


	/*
	 * Create TAP Codec.
	 */
	TAPCodec(bool verbose);

	/*
	 * Encode TAP File structure as TAP file
	 */
	bool encode(TapeFile& tapFile, string & filePath);

	/*
	 * Decode TAP file as TAP File structure
	 */
	bool decode(string &dataFileName, TapeFile& tapFile);

	bool decodeSingleFile(ifstream& fin, unsigned file_size, TapeFile& tapFile);

	bool decodeMultipleFiles(string& tapFileName, vector<TapeFile> &atomFiles);


private:

	bool mVerbose = false;
};



#endif

