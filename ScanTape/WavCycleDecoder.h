#pragma once

#ifndef WAV_CYCLE_DECODER_H
#define WAV_CYCLE_DECODER_H


#include "CycleDecoder.h"
#include "LevelDecoder.h"


class WavCycleDecoder : public CycleDecoder
{

private:

	LevelDecoder& mLevelDecoder;

	Level mLevel = Level::NoCarrierLevel;

	// Collect as many samples as possible of the same level (High or Low)
	bool getSameLevelCycles(int& nSamples);

	bool mVerbose = false;

public:

	WavCycleDecoder(int sampleFreq, LevelDecoder& levelDecoder, ArgParser& argParser);

	// Advance n samples and record the encountered no of 1/2 cycles
	int countHalfCycles(int nSamples, int& half_cycles, Frequency& lastHalfCycleFrequency);

	// Consume as many 1/2 cycles of frequency f as possible
	int consumeHalfCycles(Frequency f, int &nHalfCycles, Frequency& lastHalfCycleFrequency);

	// Stop at first occurrence of n 1/2 cycles of frequency f
	int stopOnHalfCycles(Frequency f, int nHalfCycles, double &waitingTime, Frequency &lastHalfCycleFrequency);

	// Get the next cycle (which is ether a low - F1 - or high - F2 - tone cycle)
	bool getNextCycle(CycleSample& cycleSample);

	// Wait until a cycle of a certain frequency is detected
	bool waitUntilCycle(Frequency freq, CycleSample& cycleSample);

	// Wait for a high tone (F2)
	bool  waitForTone(double minDuration, double& duration, double& waitingTime, int& highToneCycles, Frequency& lastHalfCycleFrequency);

	// Get last sampled cycle
	CycleSample getCycle();

	// Get tape time
	double getTime();

	// Save the current cycle
	bool checkpoint();

	// Roll back to a previously saved cycle
	bool rollback();
};

#endif