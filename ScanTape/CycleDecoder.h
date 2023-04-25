#pragma once

#ifndef CYCLE_DECODER_H
#define CYCLE_DECODER_H



#include "LevelDecoder.h"

#include <vector>

class CycleDecoder
{

public:

	typedef enum { F1, F2, NoCarrier, Undefined } Frequency;

	typedef enum { Low, High, Unspecified } Phase;


	

	typedef struct {
		Frequency freq;
		int sampleStart, sampleEnd;
	} CycleSample;
	typedef vector<CycleSample> CycleSamples;
	typedef CycleSamples::iterator CycleSampleIter;

private:

	bool mTracing;

	CycleSample mCycleSample = { Frequency::NoCarrier, 0, 0 };
	Frequency mPrevcycle = Frequency::NoCarrier;

	vector<CycleSample> mCycleSampleCheckpoints;

	ArgParser mArgParser;

	LevelDecoder &mLevelDecoder;

	// Min & max durations of F1 & F2 frequency low/high phases
	int mMinNSamplesF1Cycle; // Min duration of an F1 cycle
	int mMaxNSamplesF1Cycle; // Max duration of an F1 cycle
	int mMinNSamplesF2Cycle; // Min duration of an F2 cycle
	int mMaxNSamplesF2Cycle; // Max duration of an F2 cycle

	// If cycles start with a low phase but the cycle detection records cycles starting with a high
	// phase, then when there is a switch from an F1 to and F2 (or F2 to an F1) cycle, the cycle
	// stretching over the transition will be around 3T/2 where T=1/F2 and to T (for F2) and 2T (for F1)
	// as is normally the case. This corresponds to use case (1) below. If the detection instead
	// records cycles staring with a low phase, all detected cycles will always have a length of
	// either T or 2T. This corresponds to use case (1) below.
	//
	// ----    ----        --------        --------    ----    ----		High	signal level
	//     ----                    --------                ----			Low
	//     |   T   |      2T       |     2T        |   T   |   T   |	(1)
	// |  T    |    3T/2   |      2T      |   3T/2     |  T    |		(2)
	//

	int mMinNSamplesF12Cycle; // Min duration of a 3T/2 cycle where T = 1/F2
	int mMaxNSamplesF12Cycle; // Min duration of a 3T/2 cycle where T = 1/F2

	int mPhaseShift = 180; // phase [degrees] when shifting from high to low frequency - normally 180 degrees

	LevelDecoder::Level mLevel = LevelDecoder::Level::NoCarrier;

	bool getSameLevelCycles(int& nSamples);




public:

	int getPhase() { return mPhaseShift;  }

	CycleDecoder(LevelDecoder& levelDecoder, ArgParser &argParser);

	bool getNextCycle(CycleSample& cycleSample);

	bool waitUntilCycle(Frequency freq, CycleSample& cycleSample);

	/*
	 * Each data stream is preceded by 5.1 seconds of 2400 Hz lead tone (reduced to 1.1 seconds
	 * if the recording has paused in the middle of a file, or 0.9 seconds between data blocks recorded in one go).
	 *
	 * At the end of the stream is a 5.3 second, 2400 Hz trailer tone (reduced to 0.2 seconds when 
	 * pausing in the middle of a file (giving at least 1.3 seconds' delay between data blocks.)
	 * 
	 * waitForTone is used by BlockDecoder for detecting both lead and trailer tones of varying lengths.
	 * The expected length is provided as argument 'toneDuration'.
	 */
	bool waitForTone(double minDuration, double &duration, double &waitingTime, int &highToneCycles);


	// Get last sampled cycle
	CycleSample getCycle();

	bool collectCycles(Frequency freq, int nRequiredCycles, CycleSample& lastValidCycleSample, int& nCollectedCycles);
	bool collectCycles(Frequency freq, CycleSample& lastValidCycleSample, int maxCycles, int& nCollectedCycles);

	double getTime();

	// Save the current file position
	bool checkpoint();

	// Roll back to a previously saved file position
	bool rollback();


};

#endif