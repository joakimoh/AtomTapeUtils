#pragma once

#ifndef MMC_CODEC_H
#define MMC_CODEC_H

#include <vector>
#include <string>
#include "CommonTypes.h"
#include "BlockTypes.h"

using namespace std;


class MMCCodec
{

public:




	/*
	 * Create TAP Codec and initialise it with a TAP
	 * file structure. If the file is not complete,
	 * then 'complete' shall be set to false.
	 */
	MMCCodec(TAPFile& tapFile, bool verbose);

	MMCCodec(bool verbose);

	/*
	 * Encode TAP File structure as an MMC file
	 */
	bool encode(string& filePath);

	/*
	 * Decode MMC file as a TAP File structure
	 */
	bool decode(string& dataFileName);

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

