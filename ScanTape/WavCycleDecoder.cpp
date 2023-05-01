
#include "../shared/CommonTypes.h"
#include "WavCycleDecoder.h"
#include "../shared/Debug.h"
#include "ArgParser.h"
#include "../shared/WaveSampleTypes.h"

#include <iostream>

using namespace std;

// Constructor
WavCycleDecoder::WavCycleDecoder(
	int sampleFreq, LevelDecoder& levelDecoder, ArgParser& argParser
) : CycleDecoder(argParser), mLevelDecoder(levelDecoder)
{
	mTracing = argParser.tracing;

	mFS = sampleFreq;
	mTS = 1.0 / mFS;

	// Min & max durations of F1 & F2 frequency low/high phases
	mMinNSamplesF1Cycle = (int)round((1 - mArgParser.mFreqThreshold) * mFS / F1_FREQ); // Min duration of an F1 cycle
	mMaxNSamplesF1Cycle = (int)round((1 + mArgParser.mFreqThreshold) * mFS / F1_FREQ); // Max duration of an F1 cycle
	int n_samples_F1 = round(mFS / F1_FREQ);
	mMinNSamplesF2Cycle = (int)round((1 - mArgParser.mFreqThreshold) * mFS / F2_FREQ); // Min duration of an F2 cycle
	mMaxNSamplesF2Cycle = (int)round((1 + mArgParser.mFreqThreshold) * mFS / F2_FREQ); // Max duration of an F2 cycle
	int n_samples_F2 = (int)round(mFS / F2_FREQ);

	mMinNSamplesF12Cycle = (int)round((1 - mArgParser.mFreqThreshold) * 3 * mFS / (F2_FREQ * 2));// Min duration of a 3T/2 cycle where T = 1/F2
	int n_samples_F12 = (int)round(3 * mFS / (F2_FREQ * 2));
	mMaxNSamplesF12Cycle = (int)round((1 + mArgParser.mFreqThreshold) * 3 * mFS / (F2_FREQ * 2)); // Min duration of a 3T/2 cycle where T = 1/F2

	if (mTracing) {
		DBG_PRINT(DBG, "F1 with a nominal cycle duration of %d shall be in the range [%d, %d]\n", n_samples_F1, mMinNSamplesF1Cycle, mMaxNSamplesF1Cycle);
		DBG_PRINT(DBG, "F2 with a nominal cycle duration of %d shall be in the range [%d, %d]\n", n_samples_F2, mMinNSamplesF2Cycle, mMaxNSamplesF2Cycle);
		DBG_PRINT(DBG, "transitional cycles (F1->F2 or F2->F1) with a nominal cycle duration of %d shall be in the range [%d, %d]\n", n_samples_F12, mMinNSamplesF12Cycle, mMaxNSamplesF12Cycle);
	}
	mCycleSample = { Frequency::NoCarrier, 0, 0 };

}

// Save the current cycle
bool WavCycleDecoder::checkpoint()
{
	mCycleSampleCheckpoints.push_back(mCycleSample);
	mLevelDecoder.checkpoint();
	return true;
}

// Roll back to a previously saved cycle
bool WavCycleDecoder::rollback()
{
	mCycleSample = mCycleSampleCheckpoints.back();
	mCycleSampleCheckpoints.pop_back();
	mLevelDecoder.rollback();
	return true;

}

// Collect as many samples as possible of the same level (High or Low)
bool WavCycleDecoder::getSameLevelCycles(int& nSamples) {

	LevelDecoder::Level initial_level = mLevel;

	bool transition = false;
	nSamples = 0;
	int sample_no;

	for (; !transition && !mLevelDecoder.endOfSamples(); nSamples++) {
		if (!mLevelDecoder.getNextSample(mLevel, sample_no)) // can fail for too long level duration or end of of samples
			return false;
		if (mLevel != initial_level)
			transition = true;
	}

	return transition;
}

// Get the next cycle (which is ether a low - F1 - or high - F2 - tone cycle)
// Assumes the first sample is the start of a Phase
bool WavCycleDecoder::getNextCycle(CycleSample& cycleSample)
{

	int n_samples_first_phase = 0, n_samples_second_phase = 0;
	Frequency freq;

	int sample_start = mLevelDecoder.getSampleNo();

	int first_phase_level = mLevelDecoder.getLevel();

	// If initial level is neither Low nor High (i.e, NoCarrier) then stop
	if (!(first_phase_level == LevelDecoder::Level::Low || first_phase_level == LevelDecoder::Level::High)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), DBG, "Illegal %s level detected\n", _LEVEL(first_phase_level));
		return false;
	}

	// Get first phase samples
	if (!getSameLevelCycles(n_samples_first_phase))
		return false;

	int second_phase_level = mLevelDecoder.getLevel();

	// If it isn't  Low->High or High-> low cycle, then stop
	if (
		first_phase_level == LevelDecoder::Level::Low && second_phase_level != LevelDecoder::Level::High ||
		first_phase_level == LevelDecoder::Level::High && second_phase_level != LevelDecoder::Level::Low
		) {
		if (mTracing)
			DEBUG_PRINT(getTime(), DBG, "Illegal transition %s to %s detected\n", _LEVEL(first_phase_level), _LEVEL(second_phase_level));
		return false;
	}

	// Get second phase samples
	if (!getSameLevelCycles(n_samples_second_phase)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), DBG, "Failed to read second phase of cycle%s\n", "");
		return false;
	}

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
		else { // mCycleSample.freq == Frequency::NoCarrier
			if (mTracing)
				DEBUG_PRINT(getTime(), DBG, "Invalid transitional cycle of duration %d detected for a previous cycle of type %s\n", n_samples, _FREQUENCY(mCycleSample.freq));
			return false;
		}
	}
	else {
		if (mTracing) {
			DEBUG_PRINT(getTime(), DBG, "Invalid cycle of duration %d detected for a previous cycle type of %s\n", n_samples, _FREQUENCY(mCycleSample.freq));
			DEBUG_PRINT(getTime(), DBG, "[%d,%d, %d]\n", mMinNSamplesF12Cycle, n_samples, mMaxNSamplesF12Cycle);
		}
		return false;
	}

	int sample_end = mLevelDecoder.getSampleNo();


	// Add sample
	cycleSample = { freq, sample_start, sample_end };
	mCycleSample = cycleSample;

	return true;

}

// Wait until a cycle of a certain frequency is detected
bool WavCycleDecoder::waitUntilCycle(Frequency freq, CycleSample& cycleSample) {

	bool found_complete_cycle = false;
	int sample_no;
	int no_carrier_detected = false;
	int no_carrier_cycle_detected = false;

	try {

		while (!found_complete_cycle && !mLevelDecoder.endOfSamples()) {

			// Wait for a first Low value
			bool found_low_level = false;
			while (!found_low_level && mLevelDecoder.getNextSample(mLevel, sample_no)) {
				if (mLevel == LevelDecoder::Level::Low)
					found_low_level = true;
				if (mLevel == LevelDecoder::Level::NoCarrier && !no_carrier_detected) {
					no_carrier_detected = true;
				}
			}

			if (found_low_level) {

				// Try to get a complete cycle 
				if (getNextCycle(cycleSample) && (cycleSample.freq == freq || freq == Frequency::Undefined)) {
					found_complete_cycle = true;

				}
				if (cycleSample.freq == Frequency::NoCarrier && !no_carrier_cycle_detected) {
					no_carrier_cycle_detected = true;
				}
			}
		}


		return found_complete_cycle;
	}

	catch (const char* e) {
		if (mTracing)
			DBG_PRINT(ERR, "Exception while searching for a complete cycle : %s\n", e);
		return false;
	}


}

// Wait for a high tone (F2)
bool WavCycleDecoder::waitForTone(double minDuration, double& duration, double& waitingTime, int& highToneCycles) {

	bool found = false;
	CycleSample cycle_sample;

	double t_wait_start = getTime();


	// Search for lead tone of the right duration
	while (!found && !mLevelDecoder.endOfSamples()) {

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
			
		}
	}



	return found;

}

// Get last sampled cycle
CycleDecoder::CycleSample WavCycleDecoder::getCycle()
{
	return mCycleSample;
}

// Collect a specified no of cycles of a certain frequency
bool WavCycleDecoder::collectCycles(Frequency freq, int nRequiredCycles, CycleSample& lastValidCycleSample, int& nCollectedCycles) {

	CycleSample sample;
	nCollectedCycles = 0;
	for (int c = 0; c < nRequiredCycles; c++) {	
		if (!getNextCycle(sample) || sample.freq != freq)
			return false;
		lastValidCycleSample = sample;
		nCollectedCycles = c + 1;
	}

	return true;
}

// Collect a max no of cycles of a certain frequency
bool WavCycleDecoder::collectCycles(Frequency freq, CycleSample& lastValidCycleSample, int maxCycles, int& nCollectedCycles) {

	CycleSample sample;
	nCollectedCycles = 0;
	while (getNextCycle(sample) && sample.freq == freq && nCollectedCycles < maxCycles) {
		lastValidCycleSample = sample;
		nCollectedCycles++;
	}


	return true;
}

// Get tape time
double WavCycleDecoder::getTime()
{
	return mLevelDecoder.getSampleNo() * mTS;
}
