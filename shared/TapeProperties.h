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
* For Acorn Atom the file complies to the following:
* 
*	 A first block normally starts with about 4 seconds of a 2400 Hz lead tone.
*	
*	 Before the next block there seems to be 2 seconds of silence, noise or mis-shaped waves that
*	 are neither 1200 nor 2400 cycles.
*	
*	 The second and all subsequent blocks belonging to the same file starts with 2 seconds of a 2400
*	 lead tone (i.e., different than the 4 seconds for the first block).
*	
*	 Within each block there seems to be a 0.5 seconds micro lead tone between the block header and the block data.
*	 It starts with a bit shorter stop bit(8 cycles of 2400 Hz instead of 9 cycles as is normally the case) followed by
*	 0.5 sec of a 2400 tone before the data block start bit comes.
*	
*	 Between two files, there seems to up at least 2 seconds of silence, noise or malformed waves that
*	 are neither 1200 nor 2400 cycles.
*	
*	 The timing above is confirmed by the disassembly of the Acorn Atom ROM at address Fxxx
*	 - see Splitting the Atom (https://www.acornatom.nl/atom_handleidingen/splitatom/splitatom.html)
*	 
*	 The reading of a tape in Acorn Atom seems also to be quite forgiving and allows a lead tone due be as short as
*	 0.85 s (irrespectively if is for the first block or some other block). The micro tone also seems to be allowed
*	 to be of any (!) duration. And when detecting a start bit it is allowed to be preceeded by any number of
*	 high tone cycles (usually the end of a lead tone, micro tone or a stop bit).
*	 Source: Splitting the Atom
* 
* For BBC Micro the file complies to the following:
* 
*	 <4 cycles of carrier> <dummy byte 0xaa> <5.1s of carrier > <header with CRC> <data with CRC>
*		{<0.9s carrier> <header with CRC> <data with CRC } <5.3s carrier> <1.8s gap>
*	
*	 Baudrate for data is 1200 as default but can be changed to 300. Carrier is 2400 Hz.
*	 A data byte is sent as <start bit> <data bit 0> ... <data bit 7> <stop bit> where the start bit is '0'
*	 and the stop bit is '1'. '0' is encoded as either 1 cycle of 1200 Hz (at 1200 Baud) or 4 cycles (at 300 Baud)
*	 whereas '1' is encoded as either 2 cycles of 2400 Hz (at 1200 Baud) or 8 cycles (at 300 Baud).
* 
*/
class  CapturedBlockTiming{

public:
	int prelude_lead_tone_cycles; // Only for BBC Micro
	int lead_tone_cycles;
	int micro_tone_cycles;// Only for Acorn Atom
	int trailer_tone_cycles; // Only for BBC Micro
	double block_gap;
	int phase_shift = 180;
	double start;
	double end;

	void init();
	void log();
} ;

class BlockTiming {
public:

	int firstBlockPreludeLeadToneCycles = 4; // prelude lead tone duration of first block [cycles] - BBC Micro only; this is the duration of the tone preceeding the dummy byte
	double firstBlockLeadToneDuration = 4; // lead tone duration of first block [s] - BBC Micro only; this is the duration [s] of the tone following upon the dummy byte
	double otherBlockLeadToneDuration = 2; // lead tone duration of all other blocks [s]
	double microLeadToneDuration = 0.5; //  micro lead tone - Atom only; separates block header and block data  [s]
	double trailerToneDuration = 0.83; //  trailer tone;- BBC Micro only; after last tape block only [s]
	double firstBlockGap = 0.0; // Gap before the first block [s] - could be as low as zero in theory
	double blockGap = 2; // Gap between each block [s]
	double lastBlockGap = 2; // Gap after the last block [s]

	void log(string prefix);
	void log();

} ;

class TapeProperties {
public:
	double baseFreq = 1200; // Hz
	int phaseShift = 0; // Degrees [0-360]
	int baudRate = 300;
	BlockTiming minBlockTiming;
	BlockTiming nomBlockTiming;
	bool preserve = false; // If set, the original tape timing shall be used when generating UEF/CSW/WAV files

	void log();
};

const TapeProperties atomTiming {
	1201.0f, 180, 300,
	{ 0,	0.85,		0.85,	0.0,	0.0,	0.0,		0.0,	0.0 },
	{ 0,	4.0,		2.0,	0.5,	0.0,	0.0,		2.0,	2.0 },
	false
};

const TapeProperties bbmTiming {
	1201.0f, 180, 1200,
	{ 3,		2.0,		0.2,	0.0,	0.5,	0.0,		0.0,	0.0 },
	{ 4,		5.1,		0.9,	0.0,	5.3,	0.0,		0.0,	1.8 },
	false
};

const TapeProperties defaultTiming = bbmTiming;




#endif