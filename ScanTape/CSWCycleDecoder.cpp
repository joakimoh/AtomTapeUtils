#include "../shared/CommonTypes.h"
#include "CSWCycleDecoder.h"
#include "../shared/Debug.h"
#include "../shared/WaveSampleTypes.h"
#include "../shared/Utility.h"

#include <iostream>

using namespace std;

// Constructor
CSWCycleDecoder::CSWCycleDecoder(
	int sampleFreq, HalfCycle firstHalfCycle, Bytes& pulses, ArgParser& argParser, bool verbose
) : CycleDecoder(argParser), mPulses(pulses), mVerbose(verbose)
{

	// Initialise pulse data
	mPulseLevel = firstHalfCycle;
	mPulseIndex = 0;
	mSampleIndex = 0;
	int dummy;
	getPulseLength(dummy, mPulseLength); // sets mPulseLength

	mTracing = argParser.tracing;

	mFS = sampleFreq;
	mTS = 1.0 / mFS;

	// Min & max durations of F1 & F2 frequency low/high 1/2 cycles
	mMinNSamplesF1Cycle = (int)round((1 - mArgParser.freqThreshold) * mFS / F1_FREQ); // Min duration of an F1 cycle
	mMaxNSamplesF1Cycle = (int)round((1 + mArgParser.freqThreshold) * mFS / F1_FREQ); // Max duration of an F1 cycle
	int n_samples_F1 = round(mFS / F1_FREQ);
	mMinNSamplesF2Cycle = (int)round((1 - mArgParser.freqThreshold) * mFS / F2_FREQ); // Min duration of an F2 cycle
	mMaxNSamplesF2Cycle = (int)round((1 + mArgParser.freqThreshold) * mFS / F2_FREQ); // Max duration of an F2 cycle
	int n_samples_F2 = (int)round(mFS / F2_FREQ);

	mMinNSamplesF12Cycle = (int)round((1 - mArgParser.freqThreshold) * 3 * mFS / (F2_FREQ * 2));// Min duration of a 3T/2 cycle where T = 1/F2
	int n_samples_F12 = (int)round(3 * mFS / (F2_FREQ * 2));
	mMaxNSamplesF12Cycle = (int)round((1 + mArgParser.freqThreshold) * 3 * mFS / (F2_FREQ * 2)); // Min duration of a 3T/2 cycle where T = 1/F2

	mMinNSamplesF1HalfCycle = (int)round((1 - mArgParser.freqThreshold) * mFS / (F1_FREQ * 2));
	mMaxNSamplesF1HalfCycle = (int)round((1 + mArgParser.freqThreshold) * mFS / (F1_FREQ * 2));
	mMinNSamplesF2HalfCycle = (int)round((1 - mArgParser.freqThreshold) * mFS / (F2_FREQ * 2));
	mMaxNSamplesF2HalfCycle = (int)round((1 + mArgParser.freqThreshold) * mFS / (F2_FREQ * 2));

	mSamplesThresholdHalfCycle = (int)round((mFS / F1_FREQ + mFS / F2_FREQ) / 4);

	if (mVerbose) {
		printf("F1 with a nominal cycle duration of %d shall be in the range [%d, %d]\n", n_samples_F1, mMinNSamplesF1Cycle, mMaxNSamplesF1Cycle);
		printf("F2 with a nominal cycle duration of %d shall be in the range [%d, %d]\n", n_samples_F2, mMinNSamplesF2Cycle, mMaxNSamplesF2Cycle);
		printf("transitional cycles (F1->F2 or F2->F1) with a nominal cycle duration of %d shall be in the range [%d, %d]\n", n_samples_F12, mMinNSamplesF12Cycle, mMaxNSamplesF12Cycle);
	}
	mCycleSample = { Frequency::NoCarrierFrequency, 0, 0 };

}

// Save the current file position
bool CSWCycleDecoder::checkpoint()
{
	mCycleSampleCheckpoints.push_back(mCycleSample);
	PulseCheckPoint cp = { mPulseIndex , mSampleIndex, mPulseLevel, mPulseLength};
	mPulsesCheckpoints.push_back(cp);
	return true;
}

// Roll back to a previously saved file position
bool CSWCycleDecoder::rollback()
{
	mCycleSample = mCycleSampleCheckpoints.back();
	mCycleSampleCheckpoints.pop_back();

	PulseCheckPoint cp = mPulsesCheckpoints.back();
	mPulseIndex = cp.pulseIndex;
	mPulseLevel = cp.pulseLevel;
	mSampleIndex = cp.sampleIndex;
	mPulseLength = cp.pulseLength;
	mPulsesCheckpoints.pop_back();

	return true;

}

bool CSWCycleDecoder::collectCycles(
	Frequency freq, int nRequiredCycles,
	CSWCycleDecoder::CycleSample& lastValidCycleSample, int& nCollectedCycles
)
{
	CycleSample sample;
	nCollectedCycles = 0;
	for (int c = 0; c < nRequiredCycles; c++) {		
		if (!getNextCycle(sample) && sample.freq != freq) {
			return false;
		}
		nCollectedCycles = c + 1;
		lastValidCycleSample = sample;
	}

	return true;
}

bool CSWCycleDecoder::collectCycles(
	Frequency freq, CycleSample& lastValidCycleSample, int maxCycles, int& nCollectedCycles
)
{
	CycleSample sample;
	nCollectedCycles = 0;
	while (getNextCycle(sample) && sample.freq == freq && nCollectedCycles < maxCycles) {
		lastValidCycleSample = sample;
		nCollectedCycles++;
	}

	return true;
}

// Get last sampled cycle
CycleDecoder::CycleSample CSWCycleDecoder::getCycle()
{
	return mCycleSample;
}

bool CSWCycleDecoder::getPulseLength(int &nextPulseIndex, int &nextPulseLength)
{
	if (mPulseIndex < mPulses.size() && mPulses[mPulseIndex] != 0) {
		nextPulseLength = mPulses[mPulseIndex];
		nextPulseIndex = mPulseIndex + 1;
	} else
	{
		if (mPulseIndex + 4 < mPulses.size() && mPulses[mPulseIndex] == 0) {
			int long_pulse = mPulses[mPulseIndex + 1];
			long_pulse += mPulses[mPulseIndex + 2] << 8;
			long_pulse += mPulses[mPulseIndex + 3] << 16;
			long_pulse += mPulses[mPulseIndex + 4] << 24;
			nextPulseLength = long_pulse;
			nextPulseIndex = mPulseIndex + 5;
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
	// Get pulse length to mSampleIndex and advance pulse index

	if (!getPulseLength(mPulseIndex, mPulseLength)) {
		// End of pulses
		return false;
	}

	if (mVerbose && mPulseLength > 100) {
		cout << (mPulseLevel == HalfCycle::HighHalfCycle ? "HIGH" : "LOW") << " pulse of duration " << mPulseLength;
		cout << " samples (" << (double) mPulseLength * mTS << " s)\n";
	}
	// Update pulse level (invert it)
	mPulseLevel = (mPulseLevel == HalfCycle::HighHalfCycle? HalfCycle::LowHalfCycle: HalfCycle::HighHalfCycle);

	// Update sample no based on previous pulse's length
	mSampleIndex += mPulseLength;

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
int CSWCycleDecoder::countHalfCycles(int nSamples, int& half_cycles)
{
	int initial_sample_index = mSampleIndex;
	bool stop = false;

	half_cycles = 0;
	int sample_no;

	for (int n = 0; !stop; ) {
		if (!getNextPulse()) // fails if there are no more pulses
			return false;
		half_cycles++;
		if (n == 0) {
			if (mPulseLevel == HalfCycle::LowHalfCycle)
				mPhaseShift = 0;
			else
				mPhaseShift = 180;
		}
		n += mPulseLength;
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
int CSWCycleDecoder::consumeHalfCycles(Frequency f, int& nHalfCycles, Frequency& lastHalfCycleFrequency)
{
	int min_d = (f == Frequency::F1 ? mMinNSamplesF1HalfCycle : mMinNSamplesF2HalfCycle);
	int max_d = (f == Frequency::F1 ? mMaxNSamplesF1HalfCycle : mMaxNSamplesF2HalfCycle);

	nHalfCycles = 0;
	bool stop = false;
	lastHalfCycleFrequency = Frequency::UndefinedFrequency;

	for (; !stop;) {
		if (!getNextPulse())
			return false;

		// Is it of the expected duration?
		if (mPulseLength >= min_d && mPulseLength <= max_d) {
			nHalfCycles++;
		}
		else
			stop = true;
	}

	if (mPulseLength >= mMinNSamplesF1HalfCycle && mPulseLength <= mMaxNSamplesF1HalfCycle)
		lastHalfCycleFrequency = Frequency::F1;
	else if (mPulseLength >= mMinNSamplesF2HalfCycle && mPulseLength <= mMaxNSamplesF2HalfCycle)
		lastHalfCycleFrequency = Frequency::F2;
	else
		lastHalfCycleFrequency = Frequency::UndefinedFrequency;

	return true;
}

// Stop at first occurrence of n 1/2 cycles of frequency f 
int CSWCycleDecoder::stopOnHalfCycles(Frequency f, int nHalfCycles, double &waitingTime, Frequency& lastHalfCycleFrequency)
{
	int min_d = (f == Frequency::F1 ? mMinNSamplesF1HalfCycle : mMinNSamplesF2HalfCycle);
	int max_d = (f == Frequency::F1 ? mMaxNSamplesF1HalfCycle : mMaxNSamplesF2HalfCycle);

	double t_start = getTime();
	double t_end;

	lastHalfCycleFrequency = Frequency::UndefinedFrequency;

	for (int n = 0; n < nHalfCycles;) {

		t_end = getTime();

		// Get one half_cycle
		if (!getNextPulse()) {
			// End of pulses
			return false;
		}
		// Is it of the expected duration?
		if (mPulseLength >= min_d && mPulseLength <= max_d) {
			n++;
			if (n == 1) {
				if (mPulseLevel == HalfCycle::LowHalfCycle)
					mPhaseShift = 0;
				else
					mPhaseShift = 180;
				waitingTime = t_end - t_start;
			}
		} 
		else
			n = 0;
	}

	if (mPulseLength >= mMinNSamplesF1HalfCycle && mPulseLength <= mMaxNSamplesF1HalfCycle)
		lastHalfCycleFrequency = Frequency::F1;
	else if (mPulseLength >= mMinNSamplesF2HalfCycle && mPulseLength <= mMaxNSamplesF2HalfCycle)
		lastHalfCycleFrequency = Frequency::F2;
	else
		lastHalfCycleFrequency = Frequency::UndefinedFrequency;

	return true;
}

/*
 * Assumes the first sample is the start of a HalfCycle 
 */
bool CSWCycleDecoder::getNextCycle(CycleSample& cycleSample)
{

	Frequency freq;

	int sample_start = mSampleIndex;

	HalfCycle first_half_cycle_level = mPulseLevel;

	// Get first half_cycle samples
	int n_samples_first_half_cycle = mPulseLength;

	// Advance to next pulse
	if (!getNextPulse()) {
		// End of pulses
		return false;
	}
		
	// Get second half_cycle samples
	int n_samples_second_half_cycle = mPulseLength;

	int sample_end = mSampleIndex;


	// A complete cycle was detected. Now determine whether it as an F1 or F2 one

	int n_samples = n_samples_first_half_cycle + n_samples_second_half_cycle;

	if (n_samples >= mMinNSamplesF1Cycle && n_samples <= mMaxNSamplesF1Cycle) {
		// An F1 frequency CYCLE (which has a LONG duration)
		freq = Frequency::F1;
		if (mPrevcycle == Frequency::F2)
			mPhaseShift = 180; // record the half_cycle when shifting frequency)
		mPrevcycle = freq;
	}
	else if (n_samples >= mMinNSamplesF2Cycle && n_samples <= mMaxNSamplesF2Cycle) {
		// An F2 frequency CYCLE (which has a SHORT duration)
		freq = Frequency::F2;
		if (mPrevcycle == Frequency::F1)
			mPhaseShift = 180; // record the half_cycle when shifting frequency
		mPrevcycle = freq;
	}
	else if (n_samples >= mMinNSamplesF12Cycle && n_samples <= mMaxNSamplesF12Cycle) {
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
				printf("%s: Invalid transitional cycle of duration % d detected for a previous cycle of type% s\n", encodeTime(getTime()).c_str(), n_samples, _FREQUENCY(mCycleSample.freq));
			*/
			return false;
		}
	}
	else {
		/*
		if (mTracing) {
			printf("%s: Invalid cycle of duration %d detected for a previous cycle type of %s\n", encodeTime(getTime()).c_str(), n_samples, _FREQUENCY(mCycleSample.freq));
			printf("%s: [%d,%d, %d]\n", encodeTime(getTime()).c_str(), mMinNSamplesF12Cycle, n_samples, mMaxNSamplesF12Cycle);
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

bool CSWCycleDecoder::waitUntilCycle(Frequency freq, CycleSample& cycleSample) {

	bool found_complete_cycle = false;
	bool more_pulses = true;

	try {

		while (more_pulses && !found_complete_cycle && mPulseIndex < mPulses.size()) {

			// Wait for a first Low value
			bool found_low_level = false;			
			while ((more_pulses = getNextPulse()) && !found_low_level) {
				if (mPulseLevel == HalfCycle::LowHalfCycle)
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

	int n_min_half_cycles = minDuration * F2_FREQ * 2;
	int n_remaining_half_cycles;

	int t_start = getTime();

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
	return (mSampleIndex * mTS);
}
