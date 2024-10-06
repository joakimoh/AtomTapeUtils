#pragma once

#ifndef FILE_DECODER_H
#define FILE_DECODER_H

#include "BlockDecoder.h"
#include <vector>
#include "CommonTypes.h"
#include "FileBlock.h"
#include "Logging.h"




class FileDecoder
{


protected:


	Logging mDebugInfo;
	bool mCat = false;
	TapeProperties mTapeTiming;

	TargetMachine mTarget;
	BlockDecoder mBlockDecoder;

	string timeToStr(double t);

public:



	FileDecoder(
		BlockDecoder& blockDecoder,
		Logging logging, TargetMachine targetMachine, TapeProperties tapeTiming, bool catOnly = false
	);

	bool readFile(ostream& logFile, TapeFile& tapFile, string searchName);




};

#endif