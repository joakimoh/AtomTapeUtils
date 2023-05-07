#pragma once

#ifndef CYCLE_DECODER_H
#define CYCLE_DECODER_H

#include <vector>
#include "ArgParser.h"
#include "../shared/WaveSampleTypes.h"


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


	ArgParser mArgParser;

	bool mTracing;

	int mFS; // sample frequency (normally 44 100 Hz for WAV files)
	double mTS; // sample duration = 1 / sample frequency

	CycleSample mCycleSample = { Frequency::NoCarrierFrequency, 0, 0 };
	Frequency mPrevcycle = Frequency::NoCarrierFrequency;

	vector<CycleSample> mCycleSampleCheckpoints;

	// Min & max durations of F1 & F2 frequency low/high phases
	int mMinNSamplesF1Cycle; // Min duration of an F1 cycle
	int mMaxNSamplesF1Cycle; // Max duration of an F1 cycle
	int mMinNSamplesF2Cycle; // Min duration of an F2 cycle
	int mMaxNSamplesF2Cycle; // Max duration of an F2 cycle

	// If each cycle starts with a low phase but the cycle detection expect a cycle to start with a high
	// phase, then when there is a switch from an F1 to and F2 (or F2 to an F1) cycle, the detected cycle
	// stretching over a transition from a low (F1) to high (f1) tone will be around 3T/2.
	// This corresponds to use case (2) below. If the cycle detection instead
	// expects cycles to start with a low phase, then all detected cycles will always have a length of
	// either T or 2T. This corresponds to use case (1) below. The same will be the case if each cycle
	// starts with a high phase but the cycle detection expects each to start with a low phase.
	//
	// ----    ----        --------        --------    ----    ----		High	signal level
	//     ----    --------        --------        ----    ----			Low
	//     |   T   |      2T       |     2T        |   T   |   T   |	(1)
	// |  T    |    3T/2   |      2T      |   3T/2     |  T    |		(2)
	//

	int mMinNSamplesF12Cycle; // Min duration of a 3T/2 cycle where T = 1/F2
	int mMaxNSamplesF12Cycle; // Min duration of a 3T/2 cycle where T = 1/F2

	int mPhaseShift = 180; // phase [degrees] when shifting from high to low frequency - normally 180 degrees


public:

	CycleDecoder(ArgParser argParser) : mArgParser(argParser) {};

	// Get the phase (Low or High) of the current cycle
	int getPhase() { return mPhaseShift;  }

	// Get the next cycle (which is ether a low - F1 - or high - F2 - tone cycle)
	virtual bool getNextCycle(CycleSample& cycleSample) = 0;

	// Wait until a cycle of a certain frequency is detected
	virtual bool waitUntilCycle(Frequency freq, CycleSample& cycleSample) = 0;

	// Wait for a high tone (F2)
	virtual bool  waitForTone(double minDuration, double &duration, double &waitingTime, int &highToneCycles) = 0;

	// Get last sampled cycle
	virtual CycleSample getCycle() = 0;

	// Collect a specified no of cycles of a certain frequency
	virtual bool collectCycles(Frequency freq, int nRequiredCycles, CycleSample& lastValidCycleSample, int& nCollectedCycles) = 0;

	// Collect a max no of cycles of a certain frequency
	virtual bool collectCycles(Frequency freq, CycleSample& lastValidCycleSample, int maxCycles, int& nCollectedCycles) = 0;

	// Get tape time
	virtual double getTime() = 0;

	// Save the current cycle
	virtual bool checkpoint() = 0;

	// Roll back to a previously saved cycle
	virtual bool rollback() = 0;


};

#endif