#pragma once

#ifndef CSW_CYCLE_DECODER_H
#define CSW_CYCLE_DECODER_H

#include "CycleDecoder.h"
#include "../shared/CommonTypes.h"



class CSWCycleDecoder : public CycleDecoder
{

	typedef struct PulseCheckPoint_struct {
		int pulseIndex;
		int sampleIndex;
		Phase pulseLevel;
		int pulseLength;
	} PulseCheckPoint;

	typedef vector<PulseCheckPoint> PulseCheckPoints;

private:

	bool mVerbose = false;

	Bytes &mPulses;
	PulseCheckPoints mPulsesCheckpoints;

	// Pulse data
	int mPulseIndex;
	Phase mPulseLevel;
	int mSampleIndex;
	int mPulseLength;

	bool getNextPulse();

	bool getPulseLength(int &nextPulseIndex);

public:

	CSWCycleDecoder(int sampleFreq, Phase firstPhase, Bytes &Pulses, ArgParser & argParser, bool verbose);

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