#pragma once

#ifndef FILE_DECODER_H
#define FILE_DECODER_H

#include "BlockDecoder.h"
#include <vector>
#include "../shared/CommonTypes.h"
#include "FileBlock.h"




class FileDecoder
{


protected:


	ArgParser mArgParser;

	bool mTracing;
	bool mVerbose;
	TargetMachine mTarget;
	BlockDecoder mBlockDecoder;

	string timeToStr(double t);

public:



	FileDecoder(
		BlockDecoder& blockDecoder,
		ArgParser& argParser
	);

	bool readFile(ofstream& logFile, TapeFile& tapFile, string searchName);




};

#endif