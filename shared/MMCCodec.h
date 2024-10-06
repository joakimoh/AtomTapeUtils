#pragma once

#ifndef MMC_CODEC_H
#define MMC_CODEC_H

#include <vector>
#include <string>
#include "CommonTypes.h"
#include "FileBlock.h"
#include "Logging.h"

using namespace std;


class MMCCodec
{

public:




	/*
	 * Create TAP Codec and initialise it with a TAP
	 * file structure. If the file is not complete,
	 * then 'complete' shall be set to false.
	 */
	MMCCodec(Logging logging);

	/*
	 * Encode TAP File structure as an MMC file
	 */
	bool encode(TapeFile &tapeFile, string& filePath);

	/*
	 * Decode MMC file as a TAP File structure
	 */
	bool decode(string& dataFileName, TapeFile& tapeFile);


private:


	Logging mDebugInfo;

};



#endif

