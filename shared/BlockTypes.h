#ifndef BLOCK_TYPES_H
#define BLOCK_TYPES_H

#include <string>
#include "CommonTypes.h"
#include "AtomBlockTypes.h"
#include "BBMBlockTypes.h"

enum TargetMachine { BBC_MODEL_A = 0, ELECTRON = 1, BBC_MODEL_B = 2, BBC_MASTER = 3, ACORN_ATOM = 4, UNKNOWN_TARGET = 0xff };
#define _TARGET_MACHINE(x) (x==BBC_MODEL_A?"BBC_MODEL_A": \
	(x==ELECTRON?"ELECTRON":(x==BBC_MODEL_B?"BBC_MODEL_B":(x==BBC_MASTER?"BBC_MASTER":(x==ACORN_ATOM?"ACORN_ATOM":"???")))))


class FileBlock {

public:

	FileBlock(TargetMachine bt) {
		blockType = bt;
	}

	string blockName() {
		if (blockType == ACORN_ATOM)
			return atomHdr.name;
		else
			return bbmHdr.name;
	}

	TargetMachine blockType;

	union {
		ATMHdr atomHdr;
		BTMHdr bbmHdr;
	};

	vector<Byte> data;

	// Overall block timing when parsing WAV file
	double tapeStartTime = 0; // start of block
	double tapeEndTime = 0; // end of block

	// Detailed block timing - used for UEF/CSW file generation later on
	int phaseShift = 180; // half_cycle [degrees] when shifting from high to low frequency - normally 180 degrees
	int preludeToneCycles = 4; // no of high frequency cycles for prelude lead tone (BBC Micro only) - normally 4 
	int leadToneCycles = 9600; // no of high frequency cycles for lead tone (including postlude for BBC Micro)- normally 4 * 2400 = 10 080
	int microToneCycles = 1200; // no of high frequency cycles between block header and data part for Atom Block - normally 0.5 * 2400 = 1200
	int trailerToneCycles = 1992; // no of high frequency cycles after last BBC Micro tape block - normally 0.83 * 2400 = 1992
	double blockGap = 2.0; // gap after block (before the next block commence) - normally 2 s (Atom) or 3.3s (BBC Micro)
};

class ATMBlock : public FileBlock { public: ATMBlock() : FileBlock(ACORN_ATOM) {}; };
class BBMBlock : public FileBlock { public: BBMBlock() : FileBlock(BBC_MODEL_B) {}; };


typedef vector<FileBlock>::iterator ATMBlockIter;
typedef vector<FileBlock>::iterator BBMBlockIter;
typedef vector<FileBlock>::iterator FileBlockIter;

class TapeFile {

public:

	TargetMachine fileType;

	TapeFile(TargetMachine ft) { fileType = ft; }

	vector<FileBlock> blocks;

	bool complete = false; // true if all blocks were successfully detected

	int firstBlock; // first encountered block's no
	int lastBlock; // last encountered block's no

	string validFileName; // file name used in file system

	bool isBasicProgram = false; // true if the file seems to contain BASIC code

	bool validTiming = false; // true if the files timing information is valid

	int baudRate = 300;

};

class TAPFile : TapeFile { TAPFile() : TapeFile(ACORN_ATOM) {} };
class TBPFile : TapeFile { TBPFile() : TapeFile(BBC_MODEL_B) {} };

#endif