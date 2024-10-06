#include "../shared/CommonTypes.h"
#include "CSWCycleDecoder.h"
#include "../shared/Logging.h"
#include "../shared/WaveSampleTypes.h"
#include "../shared/Utility.h"
#include <iostream>
#include <cmath>

using namespace std;

// Constructor
CSWCycleDecoder::CSWCycleDecoder(
	int sampleFreq, Level firstHalfCycleLevel, Bytes& pulses, double freqThreshold, Logging logging
): CycleDecoder(sampleFreq, freqThreshold, logging), mPulses(pulses)
{

	// Initialise pulse data
	mPulseInfo.pulseLevel = firstHalfCycleLevel;
	mPulseInfo.pulseIndex = 0;
	mPulseInfo.sampleIndex = 0;
	int dummy;
	(void) getPulseLength(dummy, mPulseInfo.pulseLength); // sets mPulseInfo.pulseLength to that of first pulse

	mHalfCycle = { Frequency::NoCarrierFrequency, Level::NoCarrierLevel, 0, 0 };

}

// Save the current file position
bool CSWCycleDecoder::checkpoint()
{
	mHalfCycleCheckpoints.push_back(mHalfCycle);
	mPulsesCheckpoints.push_back(mPulseInfo);
	return true;
}

// Roll back to a previously saved file position
bool CSWCycleDecoder::rollback()
{
	mHalfCycle = mHalfCycleCheckpoints.back();
	mHalfCycleCheckpoints.pop_back();

	mPulseInfo = mPulsesCheckpoints.back();

	mPulsesCheckpoints.pop_back();

	return true;

}

// Remove checkpoint (without rolling back)
bool CSWCycleDecoder::regretCheckpoint()
{
	if (mHalfCycleCheckpoints.size() == 0)
		return false;

	mHalfCycleCheckpoints.pop_back();
	mPulsesCheckpoints.pop_back();
	return true;
}


bool CSWCycleDecoder::getPulseLength(int &nextPulseIndex, int &nextPulseLength)
{
	if (mPulseInfo.pulseIndex < mPulses.size() && mPulses[mPulseInfo.pulseIndex] != 0) {
		nextPulseLength = mPulses[mPulseInfo.pulseIndex];
		nextPulseIndex = mPulseInfo.pulseIndex + 1;
	} else
	{
		if (mPulseInfo.pulseIndex + 4 < mPulses.size() && mPulses[mPulseInfo.pulseIndex] == 0) {
			int long_pulse = mPulses[mPulseInfo.pulseIndex + 1];
			long_pulse += mPulses[mPulseInfo.pulseIndex + 2] << 8;
			long_pulse += mPulses[mPulseInfo.pulseIndex + 3] << 16;
			long_pulse += mPulses[mPulseInfo.pulseIndex + 4] << 24;
			nextPulseLength = long_pulse;
			nextPulseIndex = mPulseInfo.pulseIndex + 5;
		}
		else {
			// Error - unexpected termination of pulses
			return false;
		}
	}
	return true;
}


bool CSWCycleDecoder::getNextPulse()
{
	// Get pulse length to mPulseInfo.sampleIndex and advance pulse index

	if (!getPulseLength(mPulseInfo.pulseIndex, mPulseInfo.pulseLength)) {
		// End of pulses
		return false;
	}

	if (mDebugInfo.verbose && mPulseInfo.pulseLength > 100) {
		cout << (mPulseInfo.pulseLevel == Level::HighLevel ? "HIGH" : "LOW") << " pulse of duration " << mPulseInfo.pulseLength;
		cout << " samples (" << (double) mPulseInfo.pulseLength * mCT.tS << " s)\n";
	}
	// Update pulse level (invert it)
	mPulseInfo.pulseLevel = (mPulseInfo.pulseLevel == Level::HighLevel? Level::LowLevel: Level::HighLevel);

	// Update sample no based on previous pulse's length
	mPulseInfo.sampleIndex += mPulseInfo.pulseLength;

	// Record the frequency of the last 1/2 cycle (but only if a 1/2 cycle was detected)
	updateHalfCycleFreq(mPulseInfo.pulseLength, mPulseInfo.pulseLevel);

	return true;
}

int CSWCycleDecoder::nextPulseLength(int & pulseLength)
{
	int dummy;

	// Get length of next pulse without moving to next pulse
	if (!getPulseLength(dummy, pulseLength)) {
		// End of pulses
		return false;
	}

	return true;

}

//
// Find a window with [minthresholdCycles, maxThresholdCycles] 1/2 cycles and starting with an
// 1/2 cycle of frequency type f.
//
bool CSWCycleDecoder::detectWindow(Frequency f, int nSamples, int minThresholdCycles, int maxThresholdCycles, int& nHalfCycles)
{
	int initial_sample_index = mPulseInfo.sampleIndex;
	bool stop = false;

	nHalfCycles = 0;
	int n = 0;
	vector<int> half_cycle_durations;

	for (int n = 0; !stop; ) {

		// Get next 1/2 cycle
		if (!getNextPulse()) // fails if there are no more pulses
			return false;
		n += mPulseInfo.pulseLength;
		half_cycle_durations.push_back(mPulseInfo.pulseLength);
		nHalfCycles++;

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

		// Check if n samples has elapsed
		int next_p_len;
		if (!nextPulseLength(next_p_len))
			stop = true; // fails if there are no more pulses
		else {
			// Only continue if end of next pulse is closer to the required no of samples
			if (abs(n + next_p_len - nSamples) >= abs(n - nSamples))
				stop = true;
		}
	}

	return true;
}

// Advance n samples and record the encountered no of 1/2 cycles
int CSWCycleDecoder::countHalfCycles(int nSamples, int& nHalfCycles, int& minHalfCycleDuration, int& maxHalfCycleDuration)
{
	int initial_sample_index = mPulseInfo.sampleIndex;
	bool stop = false;
	maxHalfCycleDuration = -1;
	minHalfCycleDuration = 99999;

	nHalfCycles = 0;

	for (int n = 0; !stop; ) {

		// Get next 1/2 cycle
		if (!getNextPulse()) // fails if there are no more pulses
			return false;
		nHalfCycles++;
		n += mPulseInfo.pulseLength;

		// Check for min & max
		if (mPulseInfo.pulseLength > maxHalfCycleDuration)
			maxHalfCycleDuration = mPulseInfo.pulseLength;
		if (mPulseInfo.pulseLength < minHalfCycleDuration)
			minHalfCycleDuration = mPulseInfo.pulseLength;

		// Check if n samples has elapsed
		int next_p_len;
		if (!nextPulseLength(next_p_len))
			stop = true; // fails if there are no more pulses
		else {
			// Only continue if end of next pulse is closer to the required no of samples
			if (abs(n + next_p_len - nSamples) >= abs(n - nSamples))
				stop = true;
		}

	}

	return true;
}

// Consume as many 1/2 cycles of frequency f as possible
int CSWCycleDecoder::consumeHalfCycles(Frequency f, int& nHalfCycles)
{
	nHalfCycles = 0;
	bool stop = false;

	for (; !stop;) {

		// Get next 1/2 cycle
		if (!getNextPulse())
			return false;

		// Is it of the expected duration?
		if (strictValidHalfCycleRange(f, mPulseInfo.pulseLength)) {
			nHalfCycles++;
		}
		else
			stop = true;
	}

	return true;
}

// Stop at first occurrence of n 1/2 cycles of frequency f 
int CSWCycleDecoder::stopOnHalfCycles(Frequency f, int nHalfCycles, double &waitingTime)
{

	double t_start = getTime();
	double t_end;
	int n = 0;

	for (n = 0; n < nHalfCycles;) {

		t_end = getTime();

		// Get next 1/2 cycle
		if (!getNextPulse()) {
			// End of pulses
			return false;
		}

		// Is it of the expected duration?
		if (strictValidHalfCycleRange(f, mPulseInfo.pulseLength)) {
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

// Get the next 1/2 cycle (F1, F2 or unknown)
bool CSWCycleDecoder::advanceHalfCycle()
{

	// Get one 1/2 cycle
	if (!getNextPulse())
		return false;

	return true;
}

double CSWCycleDecoder::getTime()
{
	return (mPulseInfo.sampleIndex * mCT.tS);
}

