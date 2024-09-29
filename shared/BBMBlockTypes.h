#ifndef BBM_BLOCK_TYPES_H
#define BBM_BLOCK_TYPES_H

#include <string>
#include <cstdint>
#include "CommonTypes.h"

//
// BBC Micro Block Types
//

#define _BBM_BLOCK_ORDER(last_block,no) (last_block ? "Last" : (no == 0 ? "First":"Other"))

//
// BBC Micro Tape Block Format
// 
// <name:1-10> <load adr:4> <exec adr:4> <block no:2> <block len:2> <block flag:1> <next adr:4>
//
#define BBM_TAPE_NAME_LEN 10
typedef struct {
	// uint8_t preamble; // synchronisation byte: 0x2a
	// char name[10]; // file name - up to 10 characters terminated with 0x0 - not included as of varying size
	Byte loadAdr[4];
	Byte execAdr[4];
	Byte blockNo[2];
	Byte blockLen[2];
	Byte blockFlag; // b7 = last block, b6 = empty block, b0 = locked block
	Byte nextFileAdr[4]; // always 0x0000 for tape files (only used for paged ROMs) - therefore not  saved to BBM Block
} BBMTapeBlockHdr;
#define BBM_TAPE_BLOCK_LOAD_ADR 0
#define BBM_TAPE_BLOCK_EXEC_ADR 4
#define BBM_TAPE_BLOCK_NO 8
#define BBM_TAPE_BLOCK_LEN 10
#define BBM_TAPE_BLOCK_FLAGS 12
#define BBM_TAPE_BLOCK_NEXT_ADR 13
//
// BBC Micro File structure
// 
// One block of such a file is referred to as a BTM Block (from Bbc micro Tape Module)
// 
#define BTM_HDR_NAM_SZ 10
typedef struct BTMHdr_struct {
	char name[BTM_HDR_NAM_SZ];
	Byte loadAdr[4];
	Byte execAdr[4];
	Byte blockNo[2];
	Byte blockLen[2];
	Byte blockFlag; // b7 = last block, b6 = empty block, b0 = locked block
	bool locked; // true if block is Locked
} BTMHdr;


#endif