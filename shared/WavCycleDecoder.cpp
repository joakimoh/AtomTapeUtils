
#include "CommonTypes.h"
#include "WavCycleDecoder.h"
#include "Logging.h"
#include "WaveSampleTypes.h"
#include "Utility.h"
#include <iostream>
#include <cmath>

using namespace std;

// Constructor
WavCycleDecoder::WavCycleDecoder(
	int sampleFreq, LevelDecoder& levelDecoder, double freqThreshold, Logging logging
) : CycleDecoder(sampleFreq, freqThreshold, logging), mLevelDecoder(levelDecoder)
{

	
	mHalfCycle = { Frequency::NoCarrierFrequency, Level::NoCarrierLevel, 0, 0 };

}

// Save the current cycle
bool WavCycleDecoder::checkpoint()
{
	mHalfCycleCheckpoints.push_back(mHalfCycle);
	mLevelDecoder.checkpoint();
	return true;
}

// Roll back to a previously saved cycle
bool WavCycleDecoder::rollback()
{

	if (mHalfCycleCheckpoints.size() == 0)
		return false;

	mHalfCycle = mHalfCycleCheckpoints.back();
	mHalfCycleCheckpoints.pop_back();
	mLevelDecoder.rollback();
	return true;

}

// Remove checkpoint (without rolling back)
bool WavCycleDecoder::regretCheckpoint()
{
	if (mHalfCycleCheckpoints.size() == 0)
		return false;

	(void) mHalfCycleCheckpoints.pop_back();
	(void) mLevelDecoder.regretCheckpoint();
	return true;
}

//
// Find a window with [minthresholdCycles, maxThresholdCycles] 1/2 cycles and starting with an
// 1/2 cycle of frequency type f.
//
bool WavCycleDecoder::detectWindow(Frequency f, int nSamples, int minThresholdCycles, int maxThresholdCycles, int & nHalfCycles)
{

	mHalfCycle.freq = Frequency::UndefinedFrequency;

	nHalfCycles = 0;
	int n = 0;
	vector<int> half_cycle_durations;

	for (; n < nSamples && !mLevelDecoder.endOfSamples(); n++) {

		// Get next sample
		bool transition;
		if (!getNextSample(transition)) // can fail for too long level duration or end of of samples
			return false;

		// Check for a new 1/2 cycle
		if (transition) {
			half_cycle_durations.push_back(mHalfCycle.duration);
			nHalfCycles++;		
		}

		// Check for a completed rolling window
		if (n >= nSamples - 1) {
			// Rolling window is completely filled
			if (nHalfCycles <= minThresholdCycles || nHalfCycles >= maxThresholdCycles) {
				// Rolling window is complete but the 1/2 cycles are not the expected
				// => trim window from left until the next 1/2 cycle of frequency f is found
				//    and adjust #samples in window and the 1/2 cycle count accordingly
				bool stop = false;
				while (!stop && half_cycle_durations.size() > 0) {
					int d = half_cycle_durations.front();
					half_cycle_durations.erase(half_cycle_durations.begin());
					nHalfCycles--;
					if (n > d) n -= d; else n = 0;
					if (half_cycle_durations.size() > 0) {
						d = half_cycle_durations.front();
						if (strictValidHalfCycleRange(f, d))
							stop = true;
					}
				}
			}
		}
	}

	return true;
}


// Advance n samples and record the encountered no of 1/2 cycles
int WavCycleDecoder::countHalfCycles(
	int nSamples, int& nHalfCycles, int& minHalfCycleDuration, int& maxHalfCycleDuration,
	Frequency& dominatingFreq
)
{

	nHalfCycles = 0;
	maxHalfCycleDuration = -1;
	minHalfCycleDuration = 99999;
	dominatingFreq = Frequency::UndefinedFrequency;
	int f1_cnt = 0;
	int f2_cnt = 0;
	bool first_transition = true;

	for (int n = 0; n < nSamples && !mLevelDecoder.endOfSamples(); n++) {
		bool transition;
		if (!getNextSample(transition)) // can fail for too long level duration or end of of samples
			return false;	

		// Only count 1/2 cycles that are contained within the samples window
		if (first_transition) {
			transition = false;
			first_transition = false;
		}

		// Check for a new 1/2 cycle
		if (transition) {

			// Check for min & max
			if (mHalfCycle.duration > maxHalfCycleDuration)
				maxHalfCycleDuration = mHalfCycle.duration;
			if (mHalfCycle.duration < minHalfCycleDuration)
				minHalfCycleDuration = mHalfCycle.duration;
			nHalfCycles++;

			// Check for dominating frequency
			if (lastHalfCycleFrequency() == Frequency::F1)
				f1_cnt++;
			else if (lastHalfCycleFrequency() == Frequency::F2)
				f2_cnt++;
			if (f1_cnt > f2_cnt)
				dominatingFreq = Frequency::F1;
			else if (f2_cnt > f1_cnt)
				dominatingFreq = Frequency::F2;
			else
				dominatingFreq = Frequency::UndefinedFrequency;
		}
	}

	return true;
}

// Consume as many 1/2 cycles of frequency f as possible
int WavCycleDecoder::consumeHalfCycles(Frequency f, int &nHalfCycles)
{

	nHalfCycles = 0;
	bool stop = false;

	for (;!stop;) {

		if (!advanceHalfCycle())
			return false;

		// Is it of the expected duration?
		if (strictValidHalfCycleRange(f, mHalfCycle.duration)) {
			nHalfCycles++;
		}
		else {
			stop = true;
		}
		
	}


	return true;
}

// Stop at first occurrence of n 1/2 cycles of frequency f
int WavCycleDecoder::stopOnHalfCycles(Frequency f, int nHalfCycles, double &waitingTime)
{

	double t_start = getTime();
	double t_end;
	int n = 0;

	for (; n < nHalfCycles;) {

		t_end = getTime();	

		if (!advanceHalfCycle())
			return false;

		// Is it of the expected duration?\n
;		if (mHalfCycle.freq == f) {
			n++;
			if (n == 1) { 
				waitingTime = t_end - t_start;
			}

		}
		else
			n = 0;
	}

	return true;
}

// Collect as many samples as possible of the same level (High or Low)
bool WavCycleDecoder::advanceHalfCycle() {

	bool transition = false;

	for (; !transition && !mLevelDecoder.endOfSamples(); ) {
		if (!getNextSample(transition)) // can fail for too long level duration or end of of samples
			return false;
	}

	return transition;
}

// Get next sample and update 1/2 cycle info based on it
bool WavCycleDecoder::getNextSample(bool& transition)
{
	Level level, level_p;
	level_p = mLevelDecoder.getLevel();

	transition = false;

	int sample_no;
	if (!mLevelDecoder.getNextSample(level, sample_no)) {
		// Enf of samples
		return false;
	}

	// Update 1/2 cycle info for a transition
	if (level != level_p) {
		updateHalfCycleFreq(mHalfCycle.nSamples, level_p);
		mHalfCycle.nSamples = 1; // Also count the sample that just was read above
		transition = true;
	}
	else
		mHalfCycle.nSamples++;

	return true;
}

// Get tape time
double WavCycleDecoder::getTime()
{
	return mLevelDecoder.getSampleNo() * mCT.tS;
}


