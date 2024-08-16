#pragma once

#ifndef FILE_DECODER_H
#define FILE_DECODER_H

#include "BlockDecoder.h"
#include <vector>
#include "../shared/CommonTypes.h"
#include "BlockTypes.h"




class FileDecoder
{


protected:


	ArgParser mArgParser;

	bool mTracing;
	bool mVerbose;

	string timeToStr(double t);

public:



	FileDecoder(ArgParser& argParser);

	virtual bool readFile(ofstream& logFile, TapeFile& tapFile) = 0;




};

#endif