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


class FileBlock {

public:



	TargetMachine targetMachine;

	union {
		ATMHdr atomHdr;
		BTMHdr bbmHdr;
	};

	BlockType blockType = BlockType::Unknown;
	int blockNo = -1; // ATM header lacks block no so add it here

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


protected:
	

private:

	static bool updateAtomCRC(Word& CRC, Byte data);
	static bool updateBBMCRC(Word& CRC, Byte data);
	
	static string atomTapeBlockHdrFieldName(int offset);
	static string bbmTapeBlockHdrFieldName(int offset);

	static string bbmBlockNameFromFilename(TargetMachine targetMachine, string fn);
	static string atomBlockNameFromFilename(TargetMachine targetMachine, string fn);

	static string atomFilenameFromBlockName(TargetMachine targetMachine, string fileName);
	static string bbmFilenameFromBlockName(TargetMachine targetMachine, string fileName);

public:

	FileBlock(TargetMachine bt);
	
	bool init();
	bool init(CapturedBlockTiming& block_info);

	// Properties
	int tapeHdrSz();
	string blockName();
	uint32_t loadAdr();
	uint32_t execAdr();
	int dataSz();
	bool isBasicProgram();
	bool lastBlock();
	bool firstBlock();

	string tapeField(int n);

	bool updateCRC(Word& crc, Byte data);
	static bool updateCRC(TargetMachine targetMachine, Word& crc, Byte data);

	bool decodeTapeHdr(Bytes &name, Bytes &hdr);
	bool decodeTapeHdr(Bytes &hdr);
	bool encodeTapeHdr(Bytes &hdr);
	bool logHdr(ostream* fout);
	bool logHdr();

	bool encodeTAPHdr(
		string tapefileName, uint32_t fileLoadAdr, uint32_t loadAdr, uint32_t execAdr, uint32_t blockNo, uint32_t BlockSz
	);

	bool readTapeFileName(TargetMachine target_machine, BytesIter& data_iter, Bytes& data, Word& CRC);

	string filenameFromBlockName(string fileName);
	static string filenameFromBlockName(TargetMachine target, string fileName);

	string blockNameFromFilename(string filename);
	static string blockNameFromFilename(TargetMachine target, string filename);

};


typedef vector<FileBlock>::iterator FileBlockIter;

class TapeFile {

public:

	TargetMachine fileType;

	TapeFile(TargetMachine ft) { fileType = ft; }

	vector<FileBlock> blocks;

	bool complete = true; // true if all blocks were successfully detected

	bool corrupted = false; // true if at least one block could be corrupted (CRC was incorrect)

	int firstBlock = 0; // first encountered block's no
	int lastBlock = 0; // last encountered block's no

	string validFileName; // file name used in file system

	bool isBasicProgram = false; // true if the file seems to contain BASIC code

	bool validTiming = false; // true if the files timing information is valid

	int baudRate = 300;

	void logTAPFileHdr(ostream* fout);
	void logTAPFileHdr();
	void init();

};

#endif