
#include "../shared/CommonTypes.h"
#include "WavCycleDecoder.h"
#include "../shared/Debug.h"
#include "ArgParser.h"
#include "../shared/WaveSampleTypes.h"
#include "../shared/Utility.h"
#include <iostream>
#include <cmath>

using namespace std;

// Constructor
WavCycleDecoder::WavCycleDecoder(
	int sampleFreq, LevelDecoder& levelDecoder, ArgParser& argParser
) : CycleDecoder(sampleFreq, argParser), mLevelDecoder(levelDecoder)
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
	Level level_p = mHalfCycle.level;
	Level level;
	mHalfCycle.freq = Frequency::UndefinedFrequency;

	nHalfCycles = 0;
	int sample_no;
	int n_samples = 0;
	int half_cycle_duration = 0;
	int n = 0;
	vector<int> half_cycle_durations;

	for (; n < nSamples && !mLevelDecoder.endOfSamples(); n++) {

		// Get next sample
		if (!mLevelDecoder.getNextSample(level, sample_no)) // can fail for too long level duration or end of of samples
			return false;
		n_samples++;

		// Check for a new 1/2 cycle
		if (level != level_p) {		
			half_cycle_duration = n_samples;
			updateHalfCycleFreq(half_cycle_duration, level_p);
			half_cycle_durations.push_back(half_cycle_duration);
			n_samples = 0;
			nHalfCycles++;
			level_p = level;			
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
int WavCycleDecoder::countHalfCycles(int nSamples, int& nHalfCycles, int& minHalfCycleDuration, int& maxHalfCycleDuration)
{
	Level level, level_p;

	nHalfCycles = 0;
	int sample_no;
	int n_samples = 0;
	int half_cycle_duration = 0;
	maxHalfCycleDuration = -1;
	minHalfCycleDuration = 99999;

	level_p = mLevelDecoder.getLevel();
	for (int n = 0; n < nSamples && !mLevelDecoder.endOfSamples(); n++) {
		if (!mLevelDecoder.getNextSample(level, sample_no)) // can fail for too long level duration or end of of samples
			return false;
		n_samples++;
		// Check for a new 1/2 cycle
		if (level != level_p) {
			half_cycle_duration = n_samples;
			updateHalfCycleFreq(half_cycle_duration, level_p);
			if (half_cycle_duration > maxHalfCycleDuration)
				maxHalfCycleDuration = half_cycle_duration;
			if (half_cycle_duration < minHalfCycleDuration)
				minHalfCycleDuration = half_cycle_duration;
			n_samples = 0;
			nHalfCycles++;
			level_p = level;
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

	Level level;
	Level level_p = mLevelDecoder.getLevel();
	int n_samples = 0;

	bool transition = false;
	int sample_no;

	for (; !transition && !mLevelDecoder.endOfSamples(); n_samples++) {
		if (!mLevelDecoder.getNextSample(level, sample_no)) // can fail for too long level duration or end of of samples
			return false;
		if (level != level_p)
			transition = true;
	}

	// Record the frequency of the last 1/2 cycle (but only if a 1/2 cycle was detected)
	updateHalfCycleFreq(n_samples, level_p);

	return transition;
}

// Get tape time
double WavCycleDecoder::getTime()
{
	return mLevelDecoder.getSampleNo() * mCT.tS;
}


