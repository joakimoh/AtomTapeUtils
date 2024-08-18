#include "CycleDecoder.h"

// Record the frequency of the last 1/2 cycle (but only if a 1/2 cycle was detected)
void CycleDecoder::updateHalfCycleFreq(int half_cycle_duration, Frequency& lastHalfCycleFrequency)
{
	if (half_cycle_duration >= mMinNSamplesF2HalfCycle && half_cycle_duration <= mSamplesThresholdHalfCycle)
		lastHalfCycleFrequency = Frequency::F2;
	else if (half_cycle_duration <= mMaxNSamplesF1Cycle)
		lastHalfCycleFrequency = Frequency::F1;
	else if (half_cycle_duration > 0)
		lastHalfCycleFrequency = Frequency::UndefinedFrequency;
}

// Collect a specified no of cycles of a certain frequency
bool CycleDecoder::collectCycles(
	Frequency freq, int nRequiredCycles,
	CycleSample& lastValidCycleSample, int& nCollectedCycles
) {

	CycleSample sample;
	nCollectedCycles = 0;
	for (int c = 0; c < nRequiredCycles; c++) {
		if (!getNextCycle(sample) || sample.freq != freq)
			return false;
		lastValidCycleSample = sample;
		nCollectedCycles = c + 1;
	}

	return true;
}

// Collect a max no of cycles of a certain frequency
bool CycleDecoder::collectCycles(Frequency freq, CycleSample& lastValidCycleSample, int maxCycles, int& nCollectedCycles) {

	CycleSample sample;
	nCollectedCycles = 0;
	while (getNextCycle(sample) && sample.freq == freq && nCollectedCycles < maxCycles) {
		lastValidCycleSample = sample;
		nCollectedCycles++;
	}


	return true;
}