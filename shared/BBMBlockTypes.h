#ifndef BBM_BLOCK_TYPES_H
#define BBM_BLOCK_TYPES_H

#include <string>
#include "CommonTypes.h"

//
// BBC Micro Block Types
//

#define _BBM_BLOCK_ORDER(last_block,no) (last_block ? "Last" : (no == 0 ? "First":"Other"))

//
// BBC Micro Tape Block Format
// 
#define MAX_BBM_NAME_LEN 10
typedef struct {
	// uint8_t preamble; // synchronisation byte: 0x2a
	// char name[10]; // file name - up to 10 characters terminated with 0x0 - not included as of varying size
	Byte loadAdr[4];
	Byte execAdr[4];
	Byte blockNo[2];
	Byte blockLen[2];
	Byte blockFlag;
	Byte nextFileAdr[4]; // always 0x0000 for tape files (only used for paged ROMs) - therefore not  saved to BBM Block
} BBMTapeBlockHdr;

//
// BBC Micro File structure
// 
// One block of such a file is referred to as a BBM Block (from BBc Micro)
// 
#define BBM_MMC_HDR_NAM_SZ 10
typedef struct BBMHdr_struct {
	char name[BBM_MMC_HDR_NAM_SZ];
	Byte loadAdr[4];
	Byte execAdr[4];
	Byte blockNo[2];
	Byte blockLen[2];
	Byte blockFlag;
	bool locked; // true if block is Locked
} BBMHdr;


#endif