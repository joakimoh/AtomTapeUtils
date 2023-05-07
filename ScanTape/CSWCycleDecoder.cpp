#include "../shared/CommonTypes.h"
#include "CSWCycleDecoder.h"
#include "../shared/Debug.h"
#include "../shared/WaveSampleTypes.h"
#include "../shared/Utility.h"

#include <iostream>

using namespace std;

// Constructor
CSWCycleDecoder::CSWCycleDecoder(
	int sampleFreq, Phase firstPhase, Bytes& pulses, ArgParser& argParser, bool verbose
) : CycleDecoder(argParser), mPulses(pulses), mVerbose(verbose)
{

	// Initialise pulse data
	mPulseLevel = firstPhase;
	mPulseIndex = 0;
	mSampleIndex = 0;
	int dummy;
	getPulseLength(dummy); // sets mPulseLength

	mTracing = argParser.tracing;

	mFS = sampleFreq;
	mTS = 1.0 / mFS;

	// Min & max durations of F1 & F2 frequency low/high phases
	mMinNSamplesF1Cycle = (int)round((1 - mArgParser.freqThreshold) * mFS / F1_FREQ); // Min duration of an F1 cycle
	mMaxNSamplesF1Cycle = (int)round((1 + mArgParser.freqThreshold) * mFS / F1_FREQ); // Max duration of an F1 cycle
	int n_samples_F1 = round(mFS / F1_FREQ);
	mMinNSamplesF2Cycle = (int)round((1 - mArgParser.freqThreshold) * mFS / F2_FREQ); // Min duration of an F2 cycle
	mMaxNSamplesF2Cycle = (int)round((1 + mArgParser.freqThreshold) * mFS / F2_FREQ); // Max duration of an F2 cycle
	int n_samples_F2 = (int)round(mFS / F2_FREQ);

	mMinNSamplesF12Cycle = (int)round((1 - mArgParser.freqThreshold) * 3 * mFS / (F2_FREQ * 2));// Min duration of a 3T/2 cycle where T = 1/F2
	int n_samples_F12 = (int)round(3 * mFS / (F2_FREQ * 2));
	mMaxNSamplesF12Cycle = (int)round((1 + mArgParser.freqThreshold) * 3 * mFS / (F2_FREQ * 2)); // Min duration of a 3T/2 cycle where T = 1/F2

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

bool CSWCycleDecoder::getPulseLength(int &nextPulseIndex)
{
	if (mPulseIndex < mPulses.size() && mPulses[mPulseIndex] != 0) {
		mPulseLength = mPulses[mPulseIndex];
		nextPulseIndex = mPulseIndex + 1;
	} else
	{
		if (mPulseIndex + 4 < mPulses.size() && mPulses[mPulseIndex] == 0) {
			int long_pulse = mPulses[mPulseIndex + 1];
			long_pulse += mPulses[mPulseIndex + 2] << 8;
			long_pulse += mPulses[mPulseIndex + 3] << 16;
			long_pulse += mPulses[mPulseIndex + 4] << 24;
			mPulseLength = long_pulse;
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

	if (!getPulseLength(mPulseIndex)) {
		// End of pulses
		return false;
	}

	if (mVerbose && mPulseLength > 100) {
		cout << (mPulseLevel == Phase::HighPhase ? "HIGH" : "LOW") << " pulse of duration " << mPulseLength;
		cout << " samples (" << (double) mPulseLength * mTS << " s)\n";
	}
	// Update pulse level (invert it)
	mPulseLevel = (mPulseLevel == Phase::HighPhase? Phase::LowPhase: Phase::HighPhase);

	// Update sample no based on previous pulse's length
	mSampleIndex += mPulseLength;

	return true;
}

/*
 * Assumes the first sample is the start of a Phase 
 */
bool CSWCycleDecoder::getNextCycle(CycleSample& cycleSample)
{

	Frequency freq;

	int sample_start = mSampleIndex;

	Phase first_phase_level = mPulseLevel;

	// Get first phase samples
	int n_samples_first_phase = mPulseLength;

	// Advance to next pulse
	if (!getNextPulse()) {
		// End of pulses
		return false;
	}
		
	// Get second phase samples
	int n_samples_second_phase = mPulseLength;

	int sample_end = mSampleIndex;


	// A complete cycle was detected. Now determine whether it as an F1 or F2 one

	int n_samples = n_samples_first_phase + n_samples_second_phase;

	if (n_samples >= mMinNSamplesF1Cycle && n_samples <= mMaxNSamplesF1Cycle) {
		// An F1 frequency CYCLE (which has a LONG duration)
		freq = Frequency::F1;
		if (mPrevcycle == Frequency::F2)
			mPhaseShift = 180; // record the phase when shifting frequency)
		mPrevcycle = freq;
	}
	else if (n_samples >= mMinNSamplesF2Cycle && n_samples <= mMaxNSamplesF2Cycle) {
		// An F2 frequency CYCLE (which has a SHORT duration)
		freq = Frequency::F2;
		if (mPrevcycle == Frequency::F1)
			mPhaseShift = 180; // record the phase when shifting frequency
		mPrevcycle = freq;
	}
	else if (n_samples >= mMinNSamplesF12Cycle && n_samples <= mMaxNSamplesF12Cycle) {
		// One F1 phase followed by one F2 phase. Treat this as an F2 cycle.
		if (mCycleSample.freq == Frequency::F1) {
			freq = Frequency::F2;
			mPhaseShift = 0; // record the phase when shifting frequency
			mPrevcycle = freq;
		}
		// One F2 phase followed by one F1 phase. Treat this as an F1 cycle.
		else if (mCycleSample.freq == Frequency::F2) {
			freq = Frequency::F1;
			mPhaseShift = 0; // record the phase when shifting frequency
			mPrevcycle = freq;
		}
		else { // mCycleSample.freq == Frequency::NoCarrierFrequency
			if (mTracing)
				printf("%s: Invalid transitional cycle of duration % d detected for a previous cycle of type% s\n", encodeTime(getTime()), n_samples, _FREQUENCY(mCycleSample.freq));
			return false;
		}
	}
	else {
		if (mTracing) {
			printf("%s: Invalid cycle of duration %d detected for a previous cycle type of %s\n", encodeTime(getTime()), n_samples, _FREQUENCY(mCycleSample.freq));
			printf("%s: [%d,%d, %d]\n", encodeTime(getTime()), mMinNSamplesF12Cycle, n_samples, mMaxNSamplesF12Cycle);
		}
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
				if (mPulseLevel == Phase::LowPhase)
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
bool CSWCycleDecoder::waitForTone(double minDuration, double &duration, double& waitingTime, int& highToneCycles) {

	bool found = false;
	CycleSample cycle_sample;

	double t_wait_start = getTime();

	highToneCycles = 0;
	duration = 0;

	// Search for lead tone of the right duration
	while (!found && mPulseIndex < mPulses.size()) {

		// Wait for first cycle
		if (!waitUntilCycle(Frequency::F2, cycle_sample))
			return false;

		double t_start = getTime();	

		// Get all following cycles
		int n_cycles = 0;
		while (getNextCycle(cycle_sample) && cycle_sample.freq == Frequency::F2) {
			n_cycles++;
		}
		
		double t_end = getTime();
		duration = t_end - t_start;

		if (duration > minDuration) {
			found = true;
			highToneCycles = n_cycles + 1;
			waitingTime = t_start - t_wait_start;
			if (mVerbose)
				cout << "Tone of duration " << duration << " s detected after waiting " << waitingTime << " s...\n";
		}
	}

	

	return found;

}


double CSWCycleDecoder::getTime()
{
	return (mSampleIndex * mTS);
}
