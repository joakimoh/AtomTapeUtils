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
	 * Create TAP Codec and initialise it with a TAP
	 * file structure. If the file is not complete,
	 * then 'complete' shall be set to false.
	 */
	TAPCodec(TAPFile& tapFile, bool verbose);

	TAPCodec(bool verbose);

	/*
	 * Encode TAP File structure as TAP file
	 */
	bool encode(string & filePath);

	/*
	 * Decode TAP file as TAP File structure
	 */
	bool decode(string &dataFileName);

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
	bool mVerbose = false;
};



#endif

