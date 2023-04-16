#pragma once

#ifndef FILE_DECODER_H
#define FILE_DECODER_H

#include "BlockDecoder.h"
#include <vector>
#include "../shared/CommonTypes.h"




class FileDecoder
{


private:

	BlockDecoder mBlockDecoder;
	TAPFile mTapFile;

	ArgParser mArgParser;

	bool mTracing;


public:



	FileDecoder(BlockDecoder& blockDecoder, ArgParser &argParser);

	bool readFile(ofstream& logFile);

	bool getTAPFile(TAPFile& tapFile);




};

#endif