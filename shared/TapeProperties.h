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
*The second and all subsequent blocks belonging to the same file starts with 2 seconds of a 2400
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
* 1.7 s (irrespectively if is for the first block or some other block). The micro tone also seems to be allowed
* to be of any (!) duration. And when detecting a start bit it is allowed to be preceeded by any number of
* high tone cycles (usually the end of a lead tone, micro tone or a stop bit).
* Source: Splitting the Atom
*
* Strangely enough, the timing above doesn't work for Atomulator. Atomulator seems to need a 4s lead tone for
* every block of a tape file...
* 
* Contrary to what is stated on beebwiki, there seems to be no trailer tone whatsoever ending a block or a file.
* The lead tone for subsequent block are also longer(2 sec) than what beewiki states(0.9 / 1.1 sec).
*
* What Beebwiki states  below is probably ONLY valid for BBC micro or Acron Electron:
*
*"Each data stream is preceded by 5.1 seconds of 2400 Hz lead tone (reduced to 1.1 seconds
* if the recording has paused in the middle of a file, or 0.9 seconds between data blocks recorded in one go).
* At the end of the stream is a 5.3 second, 2400 Hz trailer tone(reduced to 0.2 seconds when
* pausing in the middle of a file (giving at least 1.3 seconds' delay between data blocks.)"
*
*/

typedef struct BlockTiming_struct {
	double firstBlockLeadToneDuration = 4; // lead tone duration of first block [s]
	double otherBlockLeadToneDuration = 2; // lead tone duration of all other blocks [s] (here Atomulator seems to require 4 s)
	double trailerToneDuration = 0.0; // trailer tone duration [s]
	double microLeadToneDuration = 0.5; //  micro lead tone (separating block header and block data) [s]

	float firstBlockGap = 0.0000136; // Gap before the first block [s] - could be as low as zero in theory
	float blockGap = 2; // Gap between each block [s]
	float lastBlockGap = 2; // Gap after the last block[s]
} BlockTiming;

typedef struct TapeProperties_struct
{
	int baudRate = 300;

	float baseFreq = 1201; // Hz

	int phase = 180; // Degrees [0-360]

	BlockTiming minBlockTiming = { 1.7,		1.5,	0.0,	0.0,	0.0,		1.5,	0.0 };
	BlockTiming nomBlockTiming = { 4.2,		4.0,	0.0,	0.5,	0.0000136,	2.0,	2.5 }; // 4 to work for Atomulator
	BlockTiming maxBlockTiming = { 999999,	999999, 999999,	999999, 999999,		999999, 999999 };
	 
	bool preserve = false; // If set, the original tape timing shall be used when generating UEF/CSW/WAV files


} TapeProperties;




#endif