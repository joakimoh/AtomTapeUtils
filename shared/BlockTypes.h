#ifndef BLOCK_TYPES_H
#define BLOCK_TYPES_H

#include <string>
#include "CommonTypes.h"
#include "AtomBlockTypes.h"
#include "BBMBlockTypes.h"

typedef enum { AtomBlock, BBCMicroBlock } FileBlockType;
typedef enum { AtomFile, BBCMicroFile } FileType;

class FileBlock {

public:

	FileBlock(FileBlockType bt) {
		blockType = bt;
	}

	string blockName() {
		if (blockType == AtomBlock)
			return atomHdr.name;
		else
			return bbmHdr.name;
	}

	FileBlockType blockType;

	union {
		ATMHdr atomHdr;
		BBMHdr bbmHdr;
	};

	vector<Byte> data;

	// Overall block timing when parsing WAV file
	double tapeStartTime = 0; // start of block
	double tapeEndTime = 0; // end of block

	// Detailed block timing - used for UEF/CSW file generation later on
	int phaseShift = 180; // half_cycle [degrees] when shifting from high to low frequency - normally 180 degrees
	int leadToneCycles = 9600; // no of high frequency cycles for lead tone - normally 4 * 2400 = 10 080
	int microToneCycles = 1200; // no of high frequency cycles between block header and data part - normally 0.5 * 2400 = 1200
	double blockGap = 2.0; // gap after block (before the next block commence) - normally 2 s
};

class ATMBlock : public FileBlock { public: ATMBlock() : FileBlock(AtomBlock) {}; };
class BBMBlock : public FileBlock { public: BBMBlock() : FileBlock(BBCMicroBlock) {}; };


typedef vector<FileBlock>::iterator ATMBlockIter;
typedef vector<FileBlock>::iterator BBMBlockIter;
typedef vector<FileBlock>::iterator FileBlockIter;

class TapeFile {

public:

	FileType fileType;

	TapeFile(FileType ft) { fileType = ft; }

	vector<FileBlock> blocks;

	bool complete = false; // true if all blocks were successfully detected

	int firstBlock; // first encountered block's no
	int lastBlock; // last encountered block's no

	string validFileName; // file name used in file system

	bool isBasicProgram = false; // true if the file seems to contain BASIC code

	bool validTiming = false; // true if the files timing information is valid

	int baudRate = 300;

};

class TAPFile : TapeFile { TAPFile() : TapeFile(AtomFile) {} };
class TBPFile : TapeFile { TBPFile() : TapeFile(BBCMicroFile) {} };

#endif