#ifndef ATOM_BLOCK_TYPES_H
#define ATOM_BLOCK_TYPES_H

#include <string>
#include <cstdint>
#include "CommonTypes.h"

//
// Acorn Atom
//

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
#define ATOM_TAPE_NAME_LEN 13
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
#define ATOM_TAPE_BLOCK_FLAGS 0
#define ATOM_TAPE_BLOCK_NO_H 1
#define ATOM_TAPE_BLOCK_NO_L 2
#define ATOM_TAPE_BLOCK_LEN 3
#define ATOM_TAPE_BLOCK_EXEC_ADR_H 4
#define ATOM_TAPE_BLOCK_EXEC_ADR_L 5
#define ATOM_TAPE_BLOCK_LOAD_ADR_H 6
#define ATOM_TAPE_BLOCK_LOAD_ADR_L 7
#define ATOM_TAPE_HDR_SZ sizeof(AtomTapeBlockHdr)

#endif