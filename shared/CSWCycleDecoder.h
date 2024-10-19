#pragma once

#ifndef CSW_CYCLE_DECODER_H
#define CSW_CYCLE_DECODER_H

#include "CycleDecoder.h"
#include "CommonTypes.h"



class CSWCycleDecoder : public CycleDecoder
{

	class PulseInfo {
	public:
		int pulseIndex;
		int sampleIndex; // sample index after the pulse has been read
		Level pulseLevel;
		int pulseLength; // pulse duration (in samples)
	} ;

	typedef vector<PulseInfo> PulseCheckPoints;

private:

	Logging mLogging;

	Bytes &mPulses;
	PulseCheckPoints mPulsesCheckpoints;

	// Pulse data
	PulseInfo mPulseInfo;

	bool getNextPulse();

	bool getPulseLength(int &nextPulseIndex, int & mPulseLength);

	int nextPulseLength(int& pulseLength);

	

public:

	CSWCycleDecoder(
		int sampleFreq, Level firstHalfCycleLevel, Bytes &Pulses, double freqThreshold, Logging logging
	);

	// Find a window with [minthresholdCycles, maxThresholdCycles] 1/2 cycles and starting with an
	// 1/2 cycle of frequency type f.
	bool detectWindow(Frequency f, int nSamples, int minThresholdCycles, int maxThresholdCycles, int& halfCycles);

	// Advance n samples and record the encountered no of 1/2 cycles
	int countHalfCycles(int nSamples, int& nHalfCycles, int& minHalfCycleDuration, int & maxHalfCycleDuration);

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