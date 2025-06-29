#ifndef BLOCK_TYPES_H
#define BLOCK_TYPES_H

#include <string>
#include "CommonTypes.h"
#include "AtomBlockTypes.h"
#include "BBMBlockTypes.h"
#include "TapeProperties.h"

enum TargetMachine { BBC_MODEL_A = 0, ELECTRON = 1, BBC_MODEL_B = 2, BBC_MASTER = 3, ACORN_ATOM = 4, UNKNOWN_TARGET = 0xff };
#define _TARGET_MACHINE(x) (x==BBC_MODEL_A?"BBC_MODEL_A": \
	(x==ELECTRON?"ELECTRON":(x==BBC_MODEL_B?"BBC_MODEL_B":(x==BBC_MASTER?"BBC_MASTER":(x==ACORN_ATOM?"ACORN_ATOM":"???")))))

enum BlockType { First = 0x1, Last = 0x2, Other = 0x4, Single = 0x3, Unknown = 0x4 };

#define _BLOCK_ORDER(x) (x==BlockType::First?"First":(x==BlockType::Last?"Last":(x==BlockType::Other?"Other":(x==BlockType::Single?"Single":"Unknown"))))

class FileHeader {

public:
	string			name;
	uint32_t		loadAdr;
	uint32_t		execAdr;
	uint32_t		size;
	TargetMachine	targetMachine;
	bool			locked = false;		// Lock status for file (locked if at least one block is locked) 

	FileHeader() {}

	FileHeader(string n, uint32_t LA, uint32_t EA, uint32_t sz, TargetMachine t) :
		name(n), loadAdr(LA), execAdr(EA), targetMachine(t), size(sz), locked(false) {}

	FileHeader(string n, uint32_t LA, uint32_t EA, uint32_t sz, TargetMachine t, bool l) :
		name(n), loadAdr(LA), execAdr(EA), targetMachine(t), size(sz), locked(l) {}

	FileHeader(TargetMachine tm, string program) : targetMachine(tm), name(program) {
		if (targetMachine <= BBC_MASTER) {
			loadAdr = 0xffff0e00;
			execAdr = 0xffff0e00;
		}
		else {
			loadAdr = 0x2900;
			execAdr = 0xc2b2;
		}
	}

	void init() { name = "???"; loadAdr = 0x0; execAdr = 0x0; size = 0x0;  targetMachine = TargetMachine::UNKNOWN_TARGET; locked = false; }
	void init(TargetMachine t) { name = "???"; loadAdr = 0x0; execAdr = 0x0; size = 0x0;  targetMachine = t; locked = false; }
};

class FileBlock {

public:

	// Block Header
	string		name;		// Block name - normally the same as a complete TapeFile's program name
	uint32_t	loadAdr;	// Load address for the block
	uint32_t	execAdr;	// Execution address for the block - normally the same as a complete TapeFile's load address
	uint16_t	no;			// Block no (the blocks of a file should normally be numbered sequentially in ascending order, starting from 0)
	uint16_t	size;		// Size of the block (normally <= 256 but could be larger)
	bool		locked = false;		// Lock status for block 
	uint32_t	nextAdr;	// Address of the following block (N/A if zero)

	// Block Data
	vector<Byte> data;		// The block data (could be empty)

	// Block Type (First, Other, Single,Last)
	BlockType blockType = BlockType::Unknown;

	// Target Machine (Acorn Atom, BBC Micro, Electron, etc.)
	TargetMachine targetMachine;

	// Block's correctness status (when read from tape)
	bool completeHdr = true;
	bool completeData = true;

	// Overall block timing (when read from tape)
	double tapeStartTime = -1; // start of block
	double tapeEndTime = -1; // end of block

	// Detailed block timing (timing from tape when applicable) - used for UEF/CSW file generation later on
	int phaseShift = 180; // half_cycle [degrees] when shifting from high to low frequency - normally 180 degrees
	int preludeToneCycles = 4; // no of high frequency cycles for prelude lead tone (BBC Micro only) - normally 4 
	int leadToneCycles = 9600; // no of high frequency cycles for lead tone (including postlude for BBC Micro)- normally 4 * 2400 = 10 080
	int microToneCycles = 1200; // no of high frequency cycles between block header and data part for Atom Block - normally 0.5 * 2400 = 1200
	int trailerToneCycles = 1992; // no of high frequency cycles after last BBC Micro tape block - normally 0.83 * 2400 = 1992
	double blockGap = 2.0; // gap after block (before the next block commence) - normally 2 s (Atom) or 3.3s (BBC Micro)


protected:


private:

	static bool updateAtomCRC(Word& CRC, Byte data);
	static bool updateBBMCRC(Word& CRC, Byte data);
	
	static string atomTapeBlockHdrFieldName(int offset);
	static string bbmTapeBlockHdrFieldName(int offset);

	bool updateFromBBMFlags(Byte flags);
	bool updateFromAtomFlags(Byte flags, Byte block_sz_M1);

	Byte crAtomFlags();
	Byte crBBMFlags();

public:

	// Expected size of a tape header
	int tapeHdrSz();

	bool firstBlock() { return (blockType == BlockType::First || blockType == BlockType::Single); }
	bool lastBlock() { return (blockType == BlockType::Last || blockType == BlockType::Single); }

	Byte crFlags();

	BlockType getBlockType();
	void setBlockType(BlockType bt);

	FileBlock(TargetMachine target);
	FileBlock(TargetMachine target, string name, uint32_t no, uint32_t loadAdr, uint32_t execAdr, uint32_t blockLen);
	
	bool init();
	bool init(CapturedBlockTiming& block_info);

	string tapeField(int n);

	bool updateCRC(Word& crc, Byte data);
	static bool updateCRC(TargetMachine targetMachine, Word& crc, Byte data);

	// Decode Atom/BBC Micro cassette format block header into Tape File header
	bool decodeTapeBlockHdr(Bytes &name, Bytes &hdr, bool limitBlockNo = false);
	
	// Encode Tape File header into Atom/BBC Micro cassette format block header
	bool encodeTapeBlockHdr(Bytes &hdr);


	bool logFileBlockHdr(ostream* fout);
	bool logFileBlockHdr();

	// Encode Tape File header from explicit parameters
	bool encodeTAPHdr(
		string tapefileName, uint32_t fileLoadAdr, uint32_t loadAdr, uint32_t execAdr, uint32_t blockNo, uint32_t BlockSz
	);
	
};


typedef vector<FileBlock>::iterator FileBlockIter;

class TapeFile {

public:

	// File header (with name, load & execution address and size)
	FileHeader	header;

	// The file blocks
	vector<FileBlock> blocks;

	int firstBlock = -1; // first encountered block's no
	int lastBlock = -1; // last encountered block's no

	// File's correctness status (when read from tape)
	bool complete = true; // true if all blocks were successfully detected
	bool corrupted = false; // true if at least one block could be corrupted (CRC was incorrect)

	bool validTiming = false; // true if the file's block timing information is valid

	int baudRate = 300;

	// Overall block timing (when read from tape)
	double tapeStartTime = -1; // start of first block
	double tapeEndTime = -1; // end of last block

public:

	TapeFile(TargetMachine targetMachine) : header("???", 0x0, 0x0, 0x0, targetMachine) {}
	TapeFile(FileHeader h) { header = h; }
	TapeFile() : header("???", 0x0, 0x0, 0x0, TargetMachine::UNKNOWN_TARGET) {}

	void logFileHdr(ostream* fout);
	void logFileHdr();
	void init();
	void init(TargetMachine targetMachine);

	//
	// Iterate over all blocks and update their block types (SINGLE, FIRST, OTHER, LAST)
	//
	// Required when the no of blocks in the tape file is not known
	// when creating the tape file (but only after creation of it)
	//
	bool setBlockTypes();

	string crValidDiscFilename(string programName);
	static string crValidDiscFilename(TargetMachine targetMachine, string programName);

	string crValidBlockName(string filename);
	static string crValidBlockName(TargetMachine target, string filename);

private:
	static string crValidBlockName(string invalidChars, int nameLen, string fn);
};

#endif