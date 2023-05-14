#pragma once

#ifndef TAPE_PROPERTIES_H
#define  TAPE_PROPERTIES_H

#include <string>
#include <map>
#include <vector>


using namespace std;

/*
* Timing for a complete File consting of a number of blocks.
* 
*A first block normally starts with about 4 seconds of a 2400 Hz lead tone.
*
*Before the next block there seems to be 2 seconds of silence, noise or mis-shaped waves that
* are neither 1200 nor 2400 cycles.
*
* The second and all subsequent blocks belonging to the same file starts with 2 seconds of a 2400
* lead tone (i.e., different than the 4 seconds for the first block).
*
* Within each block there seems to be a 0.5 seconds micro lead tone between the block header and the block data.
* It starts with a bit shorter stop bit(8 cycles of 2400 Hz instead of 9 cycles as is normally the case) followed by
* 0.5 sec of a 2400 tone before the data block start bit comes.
*
* Between two files, there seems to up at least 2 seconds of silence, noise or malformed waves that
* are neither 1200 nor 2400 cycles.
*
* The timing above is confirmed by the disassembly of the Acorn Atom ROM at address Fxxx
* - see Splitting the Atom (https://www.acornatom.nl/atom_handleidingen/splitatom/splitatom.html)
* 
* The reading of a tape in Acorn Atom seems also to be quite forgiving and allows a lead tone due be as short as
* 0.85 s (irrespectively if is for the first block or some other block). The micro tone also seems to be allowed
* to be of any (!) duration. And when detecting a start bit it is allowed to be preceeded by any number of
* high tone cycles (usually the end of a lead tone, micro tone or a stop bit).
* Source: Splitting the Atom
*
* 
*/

typedef struct BlockTiming_struct {
	double firstBlockLeadToneDuration = 4; // lead tone duration of first block [s]
	double otherBlockLeadToneDuration = 2; // lead tone duration of all other blocks [s]
	double microLeadToneDuration = 0.5; //  micro lead tone (separating block header and block data) [s]
	float firstBlockGap = 0.0; // Gap before the first block [s] - could be as low as zero in theory
	float blockGap = 2; // Gap between each block [s]
	float lastBlockGap = 2; // Gap after the last block [s]
} BlockTiming;

typedef struct TapeProperties_struct
{
	int baudRate = 300;

	float baseFreq = 1201; // Hz

	int phase = 180; // Degrees [0-360]

	BlockTiming minBlockTiming = { 0.85,	0.85,	0.0,	0.0,		0.0,	0.0 };
	BlockTiming nomBlockTiming = { 4.0,		2.0,	0.5,	0.0,		2.0,	2.0 };
	 
	bool preserve = false; // If set, the original tape timing shall be used when generating UEF/CSW/WAV files


} TapeProperties;




#endif