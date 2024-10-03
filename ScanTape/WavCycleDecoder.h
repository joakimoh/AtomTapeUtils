#pragma once

#ifndef WAV_CYCLE_DECODER_H
#define WAV_CYCLE_DECODER_H


#include "CycleDecoder.h"
#include "LevelDecoder.h"


class WavCycleDecoder : public CycleDecoder
{

private:

	LevelDecoder& mLevelDecoder;


public:

	WavCycleDecoder(int sampleFreq, LevelDecoder& levelDecoder, ArgParser& argParser);

	// Advance n samples and record the encountered no of 1/2 cycles
	int countHalfCycles(int nSamples, int& half_cycles, int& minHalfCycleDuration, int& maxHalfCycleDuration);

	// Find a window with [minthresholdCycles, maxThresholdCycles] 1/2 cycles and starting with an
	// 1/2 cycle of frequency type f.
	bool detectWindow(Frequency f, int nSamples, int minThresholdCycles, int maxThresholdCycles, int& nHalfCycles);

	// Consume as many 1/2 cycles of frequency f as possible
	int consumeHalfCycles(Frequency f, int &nHalfCycles);

	// Stop at first occurrence of n 1/2 cycles of frequency f
	int stopOnHalfCycles(Frequency f, int nHalfCycles, double &waitingTime);

	// Get the next 1/2 cycle (F1, F2 or unknown)
	bool advanceHalfCycle();


	// Get tape time
	double getTime();

	// Save the current cycle
	bool checkpoint();

	// Roll back to a previously saved cycle
	bool rollback();

	// Remove checkpoint (without rolling back)
	bool regretCheckpoint();

};

#endif