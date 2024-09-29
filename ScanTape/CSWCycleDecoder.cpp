#include "../shared/CommonTypes.h"
#include "CSWCycleDecoder.h"
#include "../shared/Debug.h"
#include "../shared/WaveSampleTypes.h"
#include "../shared/Utility.h"
#include <iostream>
#include <cmath>

using namespace std;

// Constructor
CSWCycleDecoder::CSWCycleDecoder(
	int sampleFreq, HalfCycle firstHalfCycle, Bytes& pulses, ArgParser& argParser, bool verbose
) : CycleDecoder(sampleFreq, argParser), mPulses(pulses)
{

	// Initialise pulse data
	mPulseInfo.pulseLevel = firstHalfCycle;
	mPulseInfo.pulseIndex = 0;
	mPulseInfo.sampleIndex = 0;
	int dummy;
	(void) getPulseLength(dummy, mPulseInfo.pulseLength); // sets mPulseInfo.pulseLength to that of first pulse

	mCycleSample = { Frequency::NoCarrierFrequency, 0, 0 };

}

// Save the current file position
bool CSWCycleDecoder::checkpoint()
{
	mCycleSampleCheckpoints.push_back(mCycleSample);
	mPulsesCheckpoints.push_back(mPulseInfo);
	return true;
}

// Roll back to a previously saved file position
bool CSWCycleDecoder::rollback()
{
	mCycleSample = mCycleSampleCheckpoints.back();
	mCycleSampleCheckpoints.pop_back();

	mPulseInfo = mPulsesCheckpoints.back();

	mPulsesCheckpoints.pop_back();

	return true;

}

// Remove checkpoint (without rolling back)
bool CSWCycleDecoder::regretCheckpoint()
{
	if (mCycleSampleCheckpoints.size() == 0)
		return false;

	mCycleSampleCheckpoints.pop_back();
	mPulsesCheckpoints.pop_back();
	return true;
}



// Get last sampled cycle
CycleDecoder::CycleSample CSWCycleDecoder::getCycle()
{
	return mCycleSample;
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

	if (mVerbose && mPulseInfo.pulseLength > 100) {
		cout << (mPulseInfo.pulseLevel == HalfCycle::HighHalfCycle ? "HIGH" : "LOW") << " pulse of duration " << mPulseInfo.pulseLength;
		cout << " samples (" << (double) mPulseInfo.pulseLength * mCT.tS << " s)\n";
	}
	// Update pulse level (invert it)
	mPulseInfo.pulseLevel = (mPulseInfo.pulseLevel == HalfCycle::HighHalfCycle? HalfCycle::LowHalfCycle: HalfCycle::HighHalfCycle);

	// Update sample no based on previous pulse's length
	mPulseInfo.sampleIndex += mPulseInfo.pulseLength;

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

// Advance n samples and record the encountered no of 1/2 cycles
int CSWCycleDecoder::countHalfCycles(int nSamples, int& nHalfCycles, int& maxHalfCycleDuration, Frequency& lastHalfCycleFrequency)
{
	int initial_sample_index = mPulseInfo.sampleIndex;
	bool stop = false;
	maxHalfCycleDuration = -1;

	nHalfCycles = 0;

	for (int n = 0; !stop; ) {
		if (!getNextPulse()) // fails if there are no more pulses
			return false;
		if (mPulseInfo.pulseLength > maxHalfCycleDuration)
			maxHalfCycleDuration = mPulseInfo.pulseLength;
		nHalfCycles++;
		if (n == 0) {
			if (mPulseInfo.pulseLevel == HalfCycle::LowHalfCycle)
				mPhaseShift = 0;
			else
				mPhaseShift = 180;
		}
		n += mPulseInfo.pulseLength;
		int next_p_len;
		if (!nextPulseLength(next_p_len))
			stop = true; // fails if there are no more pulses
		else {
			// Only continue if end of next pulse is closer to the required no of samples
			if (abs(n + next_p_len - nSamples) >= abs(n - nSamples))
				stop = true;
		}

	}

	// Record the frequency of the last 1/2 cycle (but only if a 1/2 cycle was detected)
	updateHalfCycleFreq(nHalfCycles, lastHalfCycleFrequency);

	return true;
}

// Consume as many 1/2 cycles of frequency f as possible
int CSWCycleDecoder::consumeHalfCycles(Frequency f, int& nHalfCycles, Frequency& lastHalfCycleFrequency)
{

	nHalfCycles = 0;
	bool stop = false;
	lastHalfCycleFrequency = Frequency::UndefinedFrequency;

	for (; !stop;) {
		if (!getNextPulse())
			return false;

		// Is it of the expected duration?
		if (strictValidHalfCycleRange(f, mPulseInfo.pulseLength)) {
			nHalfCycles++;
		}
		else
			stop = true;
	}

	// Record the frequency of the last 1/2 cycle (but only if a 1/2 cycle was detected)
	updateHalfCycleFreq(mPulseInfo.pulseLength, lastHalfCycleFrequency);

	return true;
}

// Stop at first occurrence of n 1/2 cycles of frequency f 
int CSWCycleDecoder::stopOnHalfCycles(Frequency f, int nHalfCycles, double &waitingTime, Frequency& lastHalfCycleFrequency)
{

	double t_start = getTime();
	double t_end;
	int n = 0;

	lastHalfCycleFrequency = Frequency::UndefinedFrequency;

	for (n = 0; n < nHalfCycles;) {

		t_end = getTime();

		// Get one half_cycle
		if (!getNextPulse()) {
			// End of pulses
			return false;
		}
		// Is it of the expected duration?
		if (strictValidHalfCycleRange(f, mPulseInfo.pulseLength)) {
			n++;
			if (n == 1) {
				if (mPulseInfo.pulseLevel == HalfCycle::LowHalfCycle)
					mPhaseShift = 0;
				else
					mPhaseShift = 180;
				waitingTime = t_end - t_start;
			}
		} 
		else
			n = 0;
	}

	if (n > 0)
		lastHalfCycleFrequency = f;

	return true;
}

/*
 * Assumes the first sample is the start of a HalfCycle 
 */
bool CSWCycleDecoder::getNextCycle(CycleSample& cycleSample)
{

	Frequency freq;

	int sample_start = mPulseInfo.sampleIndex;

	HalfCycle first_half_cycle_level = mPulseInfo.pulseLevel;

	// Get first half_cycle samples
	int n_samples_first_half_cycle = mPulseInfo.pulseLength;

	// Advance to next pulse
	if (!getNextPulse()) {
		// End of pulses
		return false;
	}
		
	// Get second half_cycle samples
	int n_samples_second_half_cycle = mPulseInfo.pulseLength;

	int sample_end = mPulseInfo.sampleIndex;


	// A complete cycle was detected. Now determine whether it as an F1 or F2 one

	int n_samples = n_samples_first_half_cycle + n_samples_second_half_cycle;

	if (n_samples >= mCT.mMinNSamplesF1Cycle && n_samples <= mCT.mMaxNSamplesF1Cycle) {
		// An F1 frequency CYCLE (which has a LONG duration)
		freq = Frequency::F1;
		if (mPrevcycle == Frequency::F2)
			mPhaseShift = 180; // record the half_cycle when shifting frequency)
		mPrevcycle = freq;
	}
	else if (n_samples >= mCT.mMinNSamplesF2Cycle && n_samples <= mCT.mMaxNSamplesF2Cycle) {
		// An F2 frequency CYCLE (which has a SHORT duration)
		freq = Frequency::F2;
		if (mPrevcycle == Frequency::F1)
			mPhaseShift = 180; // record the half_cycle when shifting frequency
		mPrevcycle = freq;
	}
	else if (n_samples >= mCT.mMinNSamplesF12Cycle && n_samples <= mCT.mMaxNSamplesF12Cycle) {
		// One F1 half_cycle followed by one F2 half_cycle. Treat this as an F2 cycle.
		if (mCycleSample.freq == Frequency::F1) {
			freq = Frequency::F2;
			mPhaseShift = 0; // record the half_cycle when shifting frequency
			mPrevcycle = freq;
		}
		// One F2 half_cycle followed by one F1 half_cycle. Treat this as an F1 cycle.
		else if (mCycleSample.freq == Frequency::F2) {
			freq = Frequency::F1;
			mPhaseShift = 0; // record the half_cycle when shifting frequency
			mPrevcycle = freq;
		}
		else { // mCycleSample.freq == Frequency::NoCarrierFrequency
			/*
			if (mTracing)
				printf("%s: Invalid transitional cycle of duration % d detected for a previous cycle of type% s\n", Utility::encodeTime(getTime()).c_str(), n_samples, _FREQUENCY(mCycleSample.freq));
			*/
			return false;
		}
	}
	else {
		/*
		if (mTracing) {
			printf("%s: Invalid cycle of duration %d detected for a previous cycle type of %s\n", Utility::encodeTime(getTime()).c_str(), n_samples, _FREQUENCY(mCycleSample.freq));
			printf("%s: [%d,%d, %d]\n", Utility::encodeTime(getTime()).c_str(), mMinNSamplesF12Cycle, n_samples, mMaxNSamplesF12Cycle);
		}
		*/
		return false;
	}
	

	// Add sample
	cycleSample = { freq, sample_start, sample_end };
	mCycleSample = cycleSample;

	// Advance to next pulse
	getNextPulse();

	return true;

}

// Get the next 1/2 cycle (F1, F2 or unknown)
bool CSWCycleDecoder::nextHalfCycle(Frequency& lastHalfCycleFrequency)
{
	int half_cycle_duration;

	lastHalfCycleFrequency = Frequency::UndefinedFrequency;

	// Get one 1/2 cycle
	if (!getNextPulse())
		return false;
	half_cycle_duration = mPulseInfo.pulseLength;

	// Record the frequency of the last 1/2 cycle (but only if a 1/2 cycle was detected)
	updateHalfCycleFreq(half_cycle_duration, lastHalfCycleFrequency);

	return true;
}

bool CSWCycleDecoder::waitUntilCycle(Frequency freq, CycleSample& cycleSample) {

	bool found_complete_cycle = false;
	bool more_pulses = true;

	try {

		while (more_pulses && !found_complete_cycle && mPulseInfo.pulseIndex < mPulses.size()) {

			// Wait for a first Low value
			bool found_low_level = false;			
			while ((more_pulses = getNextPulse()) && !found_low_level) {
				if (mPulseInfo.pulseLevel == HalfCycle::LowHalfCycle)
					found_low_level = true;
			}

			if (found_low_level) {

				// Try to get a complete cycle 
				if (getNextCycle(cycleSample) && (cycleSample.freq == freq || freq == Frequency::UndefinedFrequency)) {
					found_complete_cycle = true;				
				}

			}
		}


		return found_complete_cycle;
	}

	catch (const char* e) {
		printf("Exception while searching for a complete cycle : %s\n", e);
		return false;
	}


}

/*
* Wait for high frequency (F2) tone of a minimum duration
*/
bool CSWCycleDecoder::waitForTone(double minDuration, double &duration, double& waitingTime, int& highToneCycles, Frequency & lastHalfCycleFrequency) {

	int n_min_half_cycles = (int) round(minDuration * carrierFreq() * 2);
	int n_remaining_half_cycles;

	double t_start = getTime();

	if (!stopOnHalfCycles(Frequency::F2, n_min_half_cycles, waitingTime, lastHalfCycleFrequency))
		return false;

	if (!consumeHalfCycles(Frequency::F2, n_remaining_half_cycles, lastHalfCycleFrequency))
		return false;

	highToneCycles = (n_min_half_cycles + n_remaining_half_cycles) / 2;
	duration = getTime() - t_start - waitingTime;

	return true;

}

double CSWCycleDecoder::getTime()
{
	return (mPulseInfo.sampleIndex * mCT.tS);
}

