#ifndef BLOCK_TYPES_H
#define BLOCK_TYPES_H

#include <string>
#include "CommonTypes.h"

/*
	 * Atom Tape Block format
	 *
	 * Block = <header:14-26> <data:0-256><CRC:1>
	 *
	 * header = <sync:4> <name:2-14> <flag:1> <no:2> <len:1> <eadr:2> <ladr:2>
	 *
	 * sync = 4 x 0x2a
	 * name = <name chars:1-13> <0x0d:1>
	 * flag:
	 *	Bit 7 set if not the last block.
	 *	Bit 6 set if the block contains data.
	 *	Bit 5 set if not the first block.
	 * no = block number (big endian)
	 * len = data length - 1
	 * eadr = execution address (big endian)
	 * ladr = load address (big endian)
	 */
typedef struct {
	// uint8_t preamble[4]; // synchronisation bytes: 4 x 0x2a
	// char name[14]; // file name - up to 13 characters terminated with 0xd - not included as of varying size
	Byte flags; // flags
	Byte blockNoHigh;
	Byte blockNoLow;
	Byte dataLenM1;
	Byte execAdrHigh;
	Byte execAdrLow;
	Byte loadAdrHigh;
	Byte loadAdrLow;
} AtomTapeBlockHdr;



typedef enum { First = 0x1, Last = 0x2, Other = 0x4, Single = 0x3, Unknown = 0x4 } BlockType;

#define _BLOCK_ORDER(x) (x==BlockType::First?"First":(x==BlockType::Last?"Last":(x==BlockType::Other?"Other":(x==BlockType::Single?"Single":"Unknown"))))

//
//	 ATM File structure
//	
//	 Conforms to the ATM header format specified by Wouter Ras.
//
//
typedef struct {
	char name[13]; // zero-padded with '\0' at the end
	Byte loadAdrHigh; // Load address (normally 0x2900 for BASIC programs)
	Byte loadAdrLow; //
	Byte execAdrHigh; // Execution address (normally 0xb2c2 for BASIC programs)
	Byte execAdrLow; //
	Byte lenHigh; // Length in bytes of the data section (normally data is as BASIC program)
	Byte lenLow;
} ATMHdr;

typedef struct ATMBlock_struct {

	ATMHdr hdr;
	vector<Byte> data;


	// Overall block timing when parsing WAV file
	double tapeStartTime; // start of block
	double tapeEndTime; // end of block

	// Detailed block timing - used for UEF/CSW file generation later on
	int phaseShift = 180; // phase [degrees] when shifting from high to low frequency - normally 180 degrees
	int leadToneCycles; // no of high frequency cycles for lead tone - normally 4.2 * 2400 = 10 080
	int microToneCycles; // no of high frequency cycles between block header and data part - normally 0.5 * 2400 = 12 000
	int trailerToneCycles; // no of high frequency cycles for trailer tone - normally 0 (no trailer tone at all)
	double blockGap = 2.0; // gap after block (before the next block commence) - normally 2 s


} ATMBlock;

typedef vector<ATMBlock>::iterator ATMBlockIter;

typedef struct TAPFile_struct {

	vector<ATMBlock> blocks;

	bool complete = false; // true if all blocks were successfully detected

	int firstBlock; // first encountered block's no
	int lastBlock; // last encountered block's no

	string validFileName; // file name used in file system

	bool isAbcProgram = false; // true if the file seems to contain BASIC code

	bool validTiming = false; // true if the files timing information is valid

	int baudRate = 300;

} TAPFile;

#endif