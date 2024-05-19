
#include "../shared/CommonTypes.h"
#include "WavCycleDecoder.h"
#include "../shared/Debug.h"
#include "ArgParser.h"
#include "../shared/WaveSampleTypes.h"
#include "../shared/Utility.h"

#include <iostream>

using namespace std;

// Constructor
WavCycleDecoder::WavCycleDecoder(
	int sampleFreq, LevelDecoder& levelDecoder, ArgParser& argParser
) : CycleDecoder(argParser), mLevelDecoder(levelDecoder)
{
	mTracing = argParser.tracing;
	mVerbose = mArgParser.verbose;

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

// Advance n samples and record the encountered no of 1/2 cycles
int WavCycleDecoder::countHalfCycles(int nSamples, int& half_cycles)
{
	Level level_p = mLevel;

	half_cycles = 0;
	int sample_no;

	for (int n = 0; n < nSamples && !mLevelDecoder.endOfSamples(); n++) {
		if (!mLevelDecoder.getNextSample(mLevel, sample_no)) // can fail for too long level duration or end of of samples
			return false;
		if (mLevel != level_p) {
			half_cycles++;
			level_p = mLevel;
			if (half_cycles == 1) { // Decide phaseshift based on first 1/2 cycle's level
				if (mLevel == Level::LowLevel)
					mPhaseShift = 0;
				else
					mPhaseShift = 180;
			}
		}
	}
	return true;
}

// Consume as many 1/2 cycles of frequency f as possible
int WavCycleDecoder::consumeHalfCycles(Frequency f, int &nHalfCycles, Frequency &lastHalfCycleFrequency)
{
	int min_d = (f == Frequency::F1 ? mMinNSamplesF1HalfCycle : mMinNSamplesF2HalfCycle);
	int max_d = (f == Frequency::F1 ? mMaxNSamplesF1HalfCycle : mMaxNSamplesF2HalfCycle);
	int half_cycle_duration;

	nHalfCycles = 0;
	bool stop = false;
	lastHalfCycleFrequency = Frequency::UndefinedFrequency;

	for (;!stop;) {
		if (!getSameLevelCycles(half_cycle_duration))
			return false;

		// Is it of the expected duration?
		if (half_cycle_duration >= min_d && half_cycle_duration <= max_d) {
			nHalfCycles++;
		}
		else
			stop = true;
	}

	if (half_cycle_duration >= mMinNSamplesF1HalfCycle && half_cycle_duration <= mSamplesThresholdHalfCycle)
		lastHalfCycleFrequency = Frequency::F2;
	else if (half_cycle_duration <= mMaxNSamplesF1Cycle)
		lastHalfCycleFrequency = Frequency::F1;
	else
		lastHalfCycleFrequency = Frequency::UndefinedFrequency;

	return true;
}

// Stop at first occurrence of n 1/2 cycles of frequency f
int WavCycleDecoder::stopOnHalfCycles(Frequency f, int nHalfCycles, double &waitingTime, Frequency &lastHalfCycleFrequency)
{
	int half_cycle_duration = 0;
	int min_d = (f == Frequency::F1 ? mMinNSamplesF1HalfCycle : mMinNSamplesF2HalfCycle);
	int max_d = (f == Frequency::F1 ? mMaxNSamplesF1HalfCycle : mMaxNSamplesF2HalfCycle);

	double t_start = getTime();
	double t_end;
	lastHalfCycleFrequency = Frequency::UndefinedFrequency;

	for (int n = 0; n < nHalfCycles;) {

		// Get one half_cycle
		Level initial_level = mLevel;

		t_end = getTime();	

		if (!getSameLevelCycles(half_cycle_duration))
			return false;

		// Is it of the expected duration?
		if (half_cycle_duration >= min_d && half_cycle_duration <= max_d) {
			n++;
			if (n == 1) { // Decide phaseshift based on first 1/2 cycle's level
				if (initial_level == Level::LowLevel)
					mPhaseShift = 0;
				else
					mPhaseShift = 180;
				
				waitingTime = t_end - t_start;
			}
		}
		else
			n = 0;
	}

		if (half_cycle_duration >= mMinNSamplesF1HalfCycle && half_cycle_duration <= mSamplesThresholdHalfCycle)
		lastHalfCycleFrequency = Frequency::F2;
	else if (half_cycle_duration <= mMaxNSamplesF1Cycle)
		lastHalfCycleFrequency = Frequency::F1;
	else
		lastHalfCycleFrequency = Frequency::UndefinedFrequency;

	return true;
}

// Collect as many samples as possible of the same level (High or Low)
bool WavCycleDecoder::getSameLevelCycles(int& nSamples) {

	Level initial_level = mLevel;

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
// Assumes the first sample is the start of a HalfCycle
bool WavCycleDecoder::getNextCycle(CycleSample& cycleSample)
{

	int n_samples_first_half_cycle = 0, n_samples_second_half_cycle = 0;
	Frequency freq;

	int sample_start = mLevelDecoder.getSampleNo();

	int first_half_cycle_level = mLevelDecoder.getLevel();

	// If initial level is neither Low nor High (i.e, NoCarrier) then stop
	if (!(first_half_cycle_level == Level::LowLevel || first_half_cycle_level == Level::HighLevel)) {
		/*
		if (mTracing)
			printf("%s: Illegal % s level detected\n", encodeTime(getTime()).c_str(), _LEVEL(first_half_cycle_level));
		*/
		return false;
	}

	// Get first half_cycle samples
	if (!getSameLevelCycles(n_samples_first_half_cycle))
		return false;

	int second_half_cycle_level = mLevelDecoder.getLevel();

	// If it isn't  Low->High or High-> low cycle, then stop
	if (
		first_half_cycle_level == Level::LowLevel && second_half_cycle_level != Level::HighLevel ||
		first_half_cycle_level == Level::HighLevel && second_half_cycle_level != Level::LowLevel
		) {
		/*
		if (mTracing)
			printf("%s: Illegal transition %s to %s detected\n", encodeTime(getTime()).c_str(), _LEVEL(first_half_cycle_level), _LEVEL(second_half_cycle_level));
		*/
		return false;
	}

	// Get second half_cycle samples
	if (!getSameLevelCycles(n_samples_second_half_cycle)) {
		/*
		if (mTracing)
			printf("%s: Failed to read second half_cycle of cycle%s\n", encodeTime(getTime()).c_str());
		*/
		return false;
	}

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
		if (mTracing) {
			/*
			printf("%s: Invalid cycle of duration %d detected for a previous cycle type of %s\n", encodeTime(getTime()).c_str(), n_samples, _FREQUENCY(mCycleSample.freq));
			printf("%s: [%d,%d, %d]\n", encodeTime(getTime()).c_str(), mMinNSamplesF12Cycle, n_samples, mMaxNSamplesF12Cycle);
			*/
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
				if (mLevel == Level::LowLevel)
					found_low_level = true;
				if (mLevel == Level::NoCarrierLevel && !no_carrier_detected) {
					no_carrier_detected = true;
				}
			}

			if (found_low_level) {

				// Try to get a complete cycle 
				if (getNextCycle(cycleSample) && (cycleSample.freq == freq || freq == Frequency::UndefinedFrequency)) {
					found_complete_cycle = true;

				}
				if (cycleSample.freq == Frequency::NoCarrierFrequency && !no_carrier_cycle_detected) {
					no_carrier_cycle_detected = true;
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

// Wait for a high tone (F2)
bool WavCycleDecoder::waitForTone(double minDuration, double& duration, double& waitingTime, int& highToneCycles, Frequency& lastHalfCycleFrequency) {
	
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
