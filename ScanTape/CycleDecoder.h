#pragma once

#ifndef CYCLE_DECODER_H
#define CYCLE_DECODER_H

#include <vector>
#include "ArgParser.h"
#include "../shared/WaveSampleTypes.h"

class CycleSampleTiming {
public:
	// Min & max durations of F1 & F2 frequency cycles
	int mMinNSamplesF1Cycle; // Min duration of an F1 cycle
	int mMaxNSamplesF1Cycle; // Max duration of an F1 cycle
	int mMinNSamplesF2Cycle; // Min duration of an F2 cycle
	int mMaxNSamplesF2Cycle; // Max duration of an F2 cycle

	int mMinNSamplesF1HalfCycle; // Min duration of an F1 1/2 cycle
	int mMaxNSamplesF1HalfCycle; // Max duration of an F1 1/2 cycle
	int mMinNSamplesF2HalfCycle; // Min duration of an F2 1/2 cycle
	int mMaxNSamplesF2HalfCycle; // Max duration of an F2 1/2 cycle
	int mSamplesThresholdHalfCycle; // Threshold between an F1 and an F2 1/2 cycle

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

	int mMinNSamplesF12Cycle; // Min duration of a 3T/2 cycle where T = 1/F2
	int mMaxNSamplesF12Cycle; // Min duration of a 3T/2 cycle where T = 1/F2

	int fS = 44100; // sample frequency (normally 44 100 Hz for WAV files)
	double tS; // sample duration = 1 / sample frequency
	double baseFreq = 1200;

	double freqThreshold = 0.1;

	void set(int sampleFreq, double carrierFreq, double freqThreshold);
	void set(double carrierFreq);

	void log();
};

class CycleDecoder
{

public:


	typedef struct {
		Frequency freq;
		int sampleStart, sampleEnd;
	} CycleSample;

	typedef vector<CycleSample> CycleSamples;
	typedef CycleSamples::iterator CycleSampleIter;

protected:


	ArgParser &mArgParser;
	bool mTracing;
	bool mVerbose = false;

	// Relevant only for waitUntilCycle (via calls to nextCycle)
	// Not used as detection is currently 1/2 cycle based
	CycleSample mCycleSample = { Frequency::NoCarrierFrequency, 0, 0 };
	Frequency mPrevcycle = Frequency::NoCarrierFrequency;
	vector<CycleSample> mCycleSampleCheckpoints;

	CycleSampleTiming mCT;

	int mPhaseShift = 180; // half_cycle [degrees] when starting an F1/F2 cycle
	// For UEF format
	// 0 <=> cycle starts with a LOW level
	// 180 <=> cycle starts with a HIGH level
	// Normally 180 degrees


public:

	CycleDecoder(int SampleFreq, ArgParser &argParser);

	// Get the sample frequency
	int getSampleFreq() { return mCT.fS;  }

	// Get the current phaseshift (in degrees)
	int getPhaseShift() { return mPhaseShift;  }

	// Advance n samples and record the encountered no of 1/2 cycles
	virtual int countHalfCycles(int nSamples, int& half_cycles, Frequency& lastHalfCycleFrequency) = 0;

	// Consume as many 1/2 cycles of frequency f as possible
	virtual int  consumeHalfCycles(Frequency f, int &nHalfCycles, Frequency& lastHalfCycleFrequency) = 0;

	// Stop at first occurrence of n 1/2 cycles of frequency f
	virtual int stopOnHalfCycles(Frequency f, int nHalfCycles, double &waitingTime, Frequency &lastHalfCycleFrequency) = 0;

	// Get duration (in samples) of one F2 cycle
	double getF2Samples() { return (double) mCT.fS / carrierFreq();  }

	// Get duration (in sec) of one F2 cycle
	double getF2Duration() { return (double) 1 / carrierFreq(); }

	// Get the next cycle (which is ether a low - F1 - or high - F2 - tone cycle)
	virtual bool getNextCycle(CycleSample& cycleSample) = 0;

	// Get the next 1/2 cycle (F1, F2 or unknown)
	virtual bool nextHalfCycle(Frequency& lastHalfCycleFrequency) = 0;

	// Wait until a cycle of a certain frequency is detected
	virtual bool waitUntilCycle(Frequency freq, CycleSample& cycleSample) = 0;

	// Wait for a high tone (F2)
	virtual bool  waitForTone(double minDuration, double &duration, double &waitingTime, int &highToneCycles, Frequency& lastHalfCycleFrequency) = 0;

	// Get last sampled cycle
	virtual CycleSample getCycle() = 0;

	// Get tape time
	virtual double getTime() = 0;

	// Save the current cycle
	virtual bool checkpoint() = 0;

	// Roll back to a previously saved cycle
	virtual bool rollback() = 0;

	// Remove checkpoint (without rolling back)
	virtual bool regretCheckpoint() = 0;

	// Return carrier frequency [Hz]
	double carrierFreq();

	// Return carrier frequency [Hz]
	void setCarrierFreq(double carrierFreq);

protected:
	void updateHalfCycleFreq(int half_cycle_duration, Frequency& lastHalfCycleFrequency);

	// Collect a max no of cycles of a certain frequency
	bool collectCycles(Frequency freq, CycleSample& lastValidCycleSample, int maxCycles, int& nCollectedCycles);
	bool collectCycles(Frequency freq, int nRequiredCycles, CycleSample& lastValidCycleSample, int& nCollectedCycles);

};

#endif