#pragma once

#ifndef TAPE_PROPERTIES_H
#define  TAPE_PROPERTIES_H

#include <string>
#include <map>
#include <vector>


using namespace std;

typedef struct BlockTiming_struct {
	double firstBlockLeadToneDuration = 4.2; // lead tone duration of first block [s]
	double otherBlockLeadToneDuration = 2; // lead tone duration of all other blocks [s]
	double trailerToneDuration = 0.0; // trailer tone duration [s]
	double microLeadToneDuration = 0.5; //  micro lead tone (separating block header and block data) [s]

	float firstBlockGap = 0.0000136; // Gap before the first block [s]
	float otherBlockGap = 2; // Gap before all other blocks [s]
	float lastBlockGap = 2.5; // Gap after the last block[s]
} BlockTiming;

typedef struct TapeProperties_struct
{
	int baudRate = 300;

	float baseFreq = 1201; // Hz

	int phase = 180; // Degrees [0-360]

	BlockTiming minBlockTiming = { 3.8,		1.5, 0.0,		0.3, 0.0,		1.5, 0.0 };
	BlockTiming nomBlockTiming = { 4.2,		4.0, 0.0,		0.5, 0.0000136,	2.0, 2.5 }; // 4 to work for Atomulator
	BlockTiming maxBlockTiming = { 999999,	2.5, 999999,	0.8, 99999,		2.5, 999999 };
	 
	bool preserve = false; // If set, the original tape timing shall be used when generating UEF/CSW/WAV files


} TapeProperties;




#endif