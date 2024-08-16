#pragma once

#ifndef BBM_FILE_DECODER_H
#define BBM_FILE_DECODER_H

#include "BBMBlockDecoder.h"
#include <vector>
#include "../shared/CommonTypes.h"
#include "FileDecoder.h"




class BBMFileDecoder : public FileDecoder
{


private:
	BBMBlockDecoder mBlockDecoder;

public:

	BBMFileDecoder(BBMBlockDecoder& blockDecoder, ArgParser& argParser);

	bool readFile(ofstream& logFile, TapeFile& tapFile);

};

#endif