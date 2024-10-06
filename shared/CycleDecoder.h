#pragma once

#ifndef CYCLE_DECODER_H
#define CYCLE_DECODER_H

#include <vector>
#include "WaveSampleTypes.h"
#include "Logging.h"

class CycleSampleTiming {
public:

	double mMinNSamplesF1HalfCycle; // Min duration of an F1 1/2 cycle
	double mMaxNSamplesF1HalfCycle; // Max duration of an F1 1/2 cycle
	double mMinNSamplesF2HalfCycle; // Min duration of an F2 1/2 cycle
	double mMaxNSamplesF2HalfCycle; // Max duration of an F2 1/2 cycle
	double mSamplesThresholdHalfCycle; // Threshold between an F1 and an F2 1/2 cycle

	// If each cycle starts with a low half_cycle but the cycle detection expects a cycle to start with a high
	// half_cycle, then when there is a switch from an F1 to and F2 (or F2 to an F1) cycle, the detected cycle
	// stretching over a transition from a low (F1) to high (F2) tone will be around 3T/2.
	// This corresponds to use case (2) below. If the cycle detection instead
	// expects cycles to start with a low half_cycle, then all detected cycles will always have a length of
	// either T or 2T. This corresponds to use case (1) below. The same will be the case if each cycle
	// starts with a high half_cycle but the cycle detection expects each to start with a low half_cycle.
	//
	// ----    ----        --------        --------    ----    ----		High	signal level
	//     ----    --------        --------        ----    ----			Low
	//     |   T   |      2T       |     2T        |   T   |   T   |	(1)
	// |  T    |    3T/2   |      2T      |   3T/2     |  T    |		(2)
	//


	double mMinNSamplesF12HalfCycle; // Min duration of a 3T/4 1/2 cycle where T = 1/F2
	double mMaxNSamplesF12HalfCycle; // Min duration of a 3T/4 1/2 cycle where T = 1/F2

	int fS = 44100; // sample frequency (normally 44 100 Hz for WAV files)
	double tS; // sample duration = 1 / sample frequency
	double baseFreq = 1200;

	double freqThreshold = 0.1;

	void set(int sampleFreq, double carrierFreq, double freqThreshold);
	void set(double carrierFreq);

	void log();
};

class HalfCycleInfo {
public:
	Frequency freq; // Frequency of the last 1/2 cycle
	Level level; // Level of the last 1/2 cycle
	int duration; // duration of 1/2 cycle
	int phaseShift; // phaseshift [degrees] when starting an F1/F2 1/2 cycle
	int nSamples = 0; // No of samples since last transition (only used by WavTapeReader)
	string info(); // string representation of the 1/2 cycle info
};

class CycleDecoder
{

public:

	CycleSampleTiming mCT;

protected:

	Logging mDebugInfo;

	// State of the Cycle Decoder - saved when creating a checkpoint
	HalfCycleInfo mHalfCycle = { Frequency::NoCarrierFrequency, Level::NoCarrierLevel, 0, 0 };
	vector<HalfCycleInfo> mHalfCycleCheckpoints;

	// For UEF format
	// 0 <=> cycle starts with a LOW level
	// 180 <=> cycle starts with a HIGH level
	// Normally 180 degrees


public:

	HalfCycleInfo getHalfCycle() { return mHalfCycle; }

	CycleDecoder(int sampleFreq, double freqThreshold, Logging logging);

	Frequency lastHalfCycleFrequency() { return mHalfCycle.freq;  }

	// Get the sample frequency
	int getSampleFreq() { return mCT.fS;  }

	// Get the current phaseshift (in degrees)
	int getPhaseShift() { return mHalfCycle.phaseShift;  }

	// Advance n samples and record the encountered no of 1/2 cycles
	virtual int countHalfCycles(int nSamples, int& half_cycles, int &min_half_cycle_duration, int& maxHalfCycleDuration) = 0;

	// Find a window with [minthresholdCycles, maxThresholdCycles] 1/2 cycles and starting with an
	// 1/2 cycle of frequency type f.
	virtual bool detectWindow(Frequency f, int nSamples, int minThresholdCycles, int maxThresholdCycles, int& nHalfCycles) = 0;

	// Consume as many 1/2 cycles of frequency f as possible
	virtual int  consumeHalfCycles(Frequency f, int &nHalfCycles) = 0;

	// Stop at first occurrence of n 1/2 cycles of frequency f
	virtual int stopOnHalfCycles(Frequency f, int nHalfCycles, double &waitingTime) = 0;

	// Get duration (in samples) of one F2 cycle
	double getF2Samples() { return (double) mCT.fS / carrierFreq();  }

	// Get duration (in sec) of one F2 cycle
	double getF2Duration() { return (double) 1 / carrierFreq(); }

	// Get the next 1/2 cycle (F1, F2 or unknown)
	virtual bool advanceHalfCycle() = 0;

	// Get tape time
	virtual double getTime() = 0;

	// Save the current 1/2 cycle
	virtual bool checkpoint() = 0;

	// Roll back to a previously saved 1/2 cycle
	virtual bool rollback() = 0;

	// Remove checkpoint (without rolling back)
	virtual bool regretCheckpoint() = 0;

	// Return carrier frequency [Hz]
	double carrierFreq();

	// Return carrier frequency [Hz]
	void setCarrierFreq(double carrierFreq);

	// Check for valid range for either an F1 or an F2 1/2 cycle
	// The valid range for an 1/2 cycle extends to the threshold
	// between an F1 & F2 1/2 cycle.
	bool validHalfCycleRange(Frequency f, int duration);

	// Check for valid range for either an F1 or an F2 1/2 cycle
	// Only the specified tolerance will be used to validate a 1/2 cycle
	// duration.
	bool strictValidHalfCycleRange(Frequency f, int duration);

	// Determine the type of 1/2 cycle (F1, F2, F12 or unknown) - F12 has highest priority
	Frequency getHalfCycleFrequencyRange(int duration);

	// Determine the type of 1/2 cycle (F1, F2, or unknown) - F12 has lowest priority
	Frequency getHalfCycleFrequency(int duration);

protected:

	// Record the frequency of the last 1/2 cycle (but only if a 1/2 cycle was detected)
	void updateHalfCycleFreq(int halfCycleDuration, Level halfCycleLevel);

	// Get phase shift when a frequency shift occurs
	void updatePhase(Frequency f1, Frequency f2, Level level);

};

#endif