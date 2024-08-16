#pragma once

#ifndef BBM_BLOCK_DECODER_H
#define BBM_BLOCK_DECODER_H

#include <string>

#include "../shared/CommonTypes.h"
#include "../shared/BlockTypes.h"
#include "../shared/WaveSampleTypes.h"
#include "CycleDecoder.h"
#include "BlockDecoder.h"




class BBMBlockDecoder : public BlockDecoder
{

private:

	BBMTapeBlockHdr mBBMTapeBlockHdr; // read BBC Micro tape header (excluding synchronisation byte and filename)

public:

	BBMBlockDecoder(CycleDecoder& cycleDecoder, ArgParser& argParser, bool verbose);

	bool readBlock(
		double leadToneDuration, FileBlock& readBlock,
		bool& isLastBlock, int& blockNo, bool& leadToneDetected);


private:

	bool getFileName(char name[MAX_BBM_NAME_LEN], uint16_t& calc_hdr_CRC, int& len);

	void updateCRC(uint16_t &CRC, Byte data);

	string bbmTapeBlockHdrFieldName(int offset);


};

#endif