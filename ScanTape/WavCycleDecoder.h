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

	// Get the next cycle (which is ether a low - F1 - or high - F2 - tone cycle)
	bool getNextCycle(CycleSample& cycleSample);

	// Wait until a cycle of a certain frequency is detected
	bool waitUntilCycle(Frequency freq, CycleSample& cycleSample);

	// Wait for a high tone (F2)
	bool  waitForTone(double minDuration, double& duration, double& waitingTime, int& highToneCycles);

	// Get last sampled cycle
	CycleSample getCycle();

	// Collect a specified no of cycles of a certain frequency
	bool collectCycles(Frequency freq, int nRequiredCycles, CycleSample& lastValidCycleSample, int& nCollectedCycles);

	// Collect a max no of cycles of a certain frequency
	bool collectCycles(Frequency freq, CycleSample& lastValidCycleSample, int maxCycles, int& nCollectedCycles);

	// Get tape time
	double getTime();

	// Save the current cycle
	bool checkpoint();

	// Roll back to a previously saved cycle
	bool rollback();
};

#endif