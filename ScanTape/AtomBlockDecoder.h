#pragma once

#ifndef ATOM_BLOCK_DECODER_H
#define ATOM_BLOCK_DECODER_H

#include <string>

#include "../shared/CommonTypes.h"
#include "../shared/BlockTypes.h"
#include "../shared/WaveSampleTypes.h"
#include "CycleDecoder.h"
#include "BlockDecoder.h"





class AtomBlockDecoder : public BlockDecoder
{

private:

	AtomTapeBlockHdr mAtomTapeBlockHdr; // read Atom tape header (excluding synchronisation bytes and filename)

public:

	AtomBlockDecoder(CycleDecoder& cycleDecoder, ArgParser& argParser, bool verbose);

	bool readBlock(
		double leadToneDuration, FileBlock& readBlock, BlockType& block_type,
		int& blockNo, bool& leadToneDetected);

private:

	bool getFileName(char name[ATM_MMC_HDR_NAM_SZ], uint16_t& CRC, int& len);

	void updateCRC(uint16_t &CRC, Byte data);

	string atomTapeBlockHdrFieldName(int offset);


};

#endif