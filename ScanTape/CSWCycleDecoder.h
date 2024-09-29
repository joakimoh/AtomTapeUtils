#pragma once

#ifndef CSW_CYCLE_DECODER_H
#define CSW_CYCLE_DECODER_H

#include "CycleDecoder.h"
#include "../shared/CommonTypes.h"



class CSWCycleDecoder : public CycleDecoder
{

	class PulseInfo {
	public:
		int pulseIndex;
		int sampleIndex; // sample index after the pulse has been read
		HalfCycle pulseLevel;
		int pulseLength; // pulse duration (in samples)
	} ;

	typedef vector<PulseInfo> PulseCheckPoints;

private:



	Bytes &mPulses;
	PulseCheckPoints mPulsesCheckpoints;

	// Pulse data
	PulseInfo mPulseInfo;
	//int mPulseIndex;
	//int mSampleIndex;
	//HalfCycle mPulseLevel;
	//int mPulseLength;

	bool getNextPulse();

	bool getPulseLength(int &nextPulseIndex, int & mPulseLength);

	int nextPulseLength(int& pulseLength);

	

public:

	CSWCycleDecoder(int sampleFreq, HalfCycle firstHalfCycle, Bytes &Pulses, ArgParser & argParser, bool verbose);

	// Advance n samples and record the encountered no of 1/2 cycles
	int countHalfCycles(int nSamples, int& nHalfCycles, int & maxHalfCycleDuration, Frequency& lastHalfCycleFrequency);

	// Consume as many 1/2 cycles of frequency f as possible
	int consumeHalfCycles(Frequency f, int &nHalfCycles, Frequency& lastHalfCycleFrequency);

	// Stop at first occurrence of n 1/2 cycles of frequency f
	int stopOnHalfCycles(Frequency f, int nHalfCycles, double &waitingTime, Frequency &lastHalfCycleFrequency);

	// Get the next cycle (which is ether a low - F1 - or high - F2 - tone cycle)
	bool getNextCycle(CycleSample& cycleSample);

	// Get the next 1/2 cycle (F1, F2 or unknown)
	bool nextHalfCycle(Frequency& lastHalfCycleFrequency);

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

	// Remove checkpoint (without rolling back)
	bool regretCheckpoint();

};

#endif