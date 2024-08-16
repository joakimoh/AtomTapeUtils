#pragma once

#ifndef ATOM_FILE_DECODER_H
#define ATOM_FILE_DECODER_H

#include "AtomBlockDecoder.h"
#include <vector>
#include "../shared/CommonTypes.h"
#include "FileDecoder.h"




class AtomFileDecoder : public FileDecoder
{


private:
	AtomBlockDecoder mBlockDecoder;


public:



	AtomFileDecoder(AtomBlockDecoder& blockDecoder, ArgParser& argParser);

	bool readFile(ofstream& logFile, TapeFile& tapFile);




};

#endif