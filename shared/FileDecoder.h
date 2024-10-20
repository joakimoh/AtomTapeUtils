#pragma once

#ifndef FILE_DECODER_H
#define FILE_DECODER_H

#include "BlockDecoder.h"
#include <vector>
#include "CommonTypes.h"
#include "FileBlock.h"
#include "Logging.h"
#include <sstream>


enum FileReadStatus {
	OK = 0x0, MISSING_BLOCKS = 0x1, INCOMPLETE_BLOCKS = 0x2, INCOMPLETE = 0x3,
	CORRUPTED_BLOCKS = 0x4, END_OF_TAPE = 0x8
};

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

	static string readFileStatus(FileReadStatus& readStatus) {
		stringstream s;
		string prefix = "A";
		if (readStatus & MISSING_BLOCKS) {
			s << prefix << "t least one block is missing";
			prefix = ", and a";
		}
		if (readStatus & INCOMPLETE_BLOCKS) {
			s << prefix << "t least one block is incomplete";
			prefix = ", and a";
		}
		if (readStatus & CORRUPTED_BLOCKS) {
			s << prefix << "t least one block is corrupted (incorrect CRC)";
			prefix = ", and a";
		}

		return s.str();
	}

	FileDecoder(
		BlockDecoder& blockDecoder,
		Logging logging, TargetMachine targetMachine, TapeProperties tapeTiming, bool catOnly = false
	);

	bool readFile(ostream& logFile, TapeFile& tapFile, string searchName, FileReadStatus& readStatus);




};

#endif