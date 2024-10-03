#include "CycleDecoder.h"
#include <iostream>
#include <cmath>
#include "../shared/Utility.h"
#include "../shared/Debug.h"

void CycleSampleTiming::log()
{
	double f1 = baseFreq;
	double f2 = baseFreq * 2;
	int n_samples_F1 = (int)round(fS / f1);
	int n_samples_F2 = (int)round(fS / f2);
	int n_samples_F12 = (int)round(3 * fS / (f2 * 2));

	//mMinNSamplesF1HalfCycle = 2;

	cout << dec;

	cout << "Sampling Frequency: " << this->fS << " [Hz]\n";
	cout << "Sampling period: " << this->tS << " [s]\n";
	cout << "Frequency threshold: " << this->freqThreshold << "\n";
	cout << "Base Frequency: " << this->baseFreq << " [Hz]\n";
	
	cout << "Valid F1 1/2 cycles: [" << this->mMinNSamplesF1HalfCycle << " (" << this->mSamplesThresholdHalfCycle << ")" << 
		", " << n_samples_F1 / 2 << ", " << this->mMaxNSamplesF1HalfCycle <<  "]\n";

	cout << "Valid F2 1/2 cycles: [" << this->mMinNSamplesF2HalfCycle << ", " << n_samples_F2 / 2 << ", " <<
		this->mMaxNSamplesF2HalfCycle << " (" << this->mSamplesThresholdHalfCycle << ")" << "]\n";

	cout << "Valid F12 1/2 cycles: [" << this->mMinNSamplesF12HalfCycle << ", " << n_samples_F2 / 2 << ", " <<
		this->mMaxNSamplesF12HalfCycle <<  "]\n";


}


void CycleSampleTiming::set(double carrierFreq)
{
	set(fS, carrierFreq, freqThreshold);
}

void CycleSampleTiming::set(int sampleFreq, double carrierFreq, double freqTH)
{
	baseFreq = carrierFreq / 2;
	fS = sampleFreq;
	freqThreshold = freqTH;

	tS = 1.0 / fS;
	double f1 = baseFreq;
	double f2 = baseFreq * 2;

	mMinNSamplesF1HalfCycle = (int)round((1 - freqThreshold) * fS / (f1 * 2)); // Min duration of an F1 1/2 cycle
	mMaxNSamplesF1HalfCycle = (int)round((1 + freqThreshold) * fS / (f1 * 2)); // Max duration of an F1 1/2 cycle
	mMinNSamplesF2HalfCycle = (int)round((1 - freqThreshold) * fS / (f2 * 2)); // Min duration of an F2 1/2 cycle
	mMaxNSamplesF2HalfCycle = (int)round((1 + freqThreshold) * fS / (f2 * 2)); // Max duration of an F2 1/2 cycle

	mMinNSamplesF12HalfCycle = (int)round((1 - freqThreshold) * 3 * fS / (f2 * 4));// Min duration of a 3T/4 1/2 cycle where T = 1/F2	
	mMaxNSamplesF12HalfCycle = (int)round((1 + freqThreshold) * 3 * fS / (f2 * 4)); // Min duration of a 3T/4 1/2 cycle where T = 1/F2

	//mSamplesThresholdHalfCycle = ((double) fS / f1 + (double) fS / f2) / 4;
	mSamplesThresholdHalfCycle = double (mMaxNSamplesF2HalfCycle + mMinNSamplesF1HalfCycle) / 2;

}

CycleDecoder::CycleDecoder(int sampleFreq, ArgParser &argParser) : mArgParser(argParser), mTracing(argParser.tracing),
mVerbose(argParser.verbose)
{
	mCT.set(sampleFreq, F2_FREQ, mArgParser.freqThreshold);
	
}

// Get phase shift when a frequency shift occurs
// Only checked for when transitioning from an F2 1/2 cycle to either an F1 or F12 1/2 cycle
void CycleDecoder::updatePhase(Frequency f1, Frequency f2, Level f2Level)
{
	if (f1 == f2 || f1 != Frequency::F2 || f2 == Frequency::UndefinedFrequency || f2 == Frequency::NoCarrierFrequency)
		return;

	if (f2Level == Level::HighLevel) {
		if (f1 == Frequency::F2 && f2 == Frequency::F1)
			mHalfCycle.phaseShift = 0;
		else if (f1 == Frequency::F2 && f2 == Frequency::F12)
			mHalfCycle.phaseShift = 90;
	}
	else if (f2Level == Level::LowLevel) {
		if (f1 == Frequency::F2 && f2 == Frequency::F1)
			mHalfCycle.phaseShift = 180;
		else if (f1 == Frequency::F2 && f2 == Frequency::F12)
			mHalfCycle.phaseShift = 270;
	}

}

// Record the frequency of the last 1/2 cycle (but only if a 1/2 cycle was detected)
void CycleDecoder::updateHalfCycleFreq(int halfCycleDuration, Level halfCycleLevel)
{
	HalfCycleInfo half_cycle = mHalfCycle;
	Frequency strict_f;

	mHalfCycle.level = halfCycleLevel;
	mHalfCycle.duration = halfCycleDuration;
	if (halfCycleDuration >= mCT.mMinNSamplesF2HalfCycle && halfCycleDuration <= mCT.mMaxNSamplesF2HalfCycle) {
		updatePhase(mHalfCycle.freq, Frequency::F2, halfCycleLevel); // record the phase if shifting frequency
		mHalfCycle.freq = Frequency::F2;
		strict_f = Frequency::F2;
	}
	else if (halfCycleDuration > mCT.mMinNSamplesF1HalfCycle && halfCycleDuration <= mCT.mMaxNSamplesF1HalfCycle) {
		updatePhase(mHalfCycle.freq, Frequency::F1, halfCycleLevel); // record the phase if shifting frequency
		mHalfCycle.freq = Frequency::F1;
		strict_f = Frequency::F1;
	}
	else if (halfCycleDuration >= mCT.mMinNSamplesF12HalfCycle && halfCycleDuration <= mCT.mMaxNSamplesF12HalfCycle) {
		if (mHalfCycle.freq == Frequency::F1) { // One F1 1/2 cycle followed by one F12 1/2 cycle. Treat this as an F2 cycle.
			updatePhase(mHalfCycle.freq, Frequency::F12, halfCycleLevel); // record the phase if shifting frequency
			mHalfCycle.freq = Frequency::F2;
			strict_f = Frequency::F12;
		}	
		else if (mHalfCycle.freq == Frequency::F2) { // One F2 1/2 cycle followed by one F12 1/2 cycle. Treat this as an F1 cycle.
			updatePhase(mHalfCycle.freq, Frequency::F12, halfCycleLevel); // record the phase if shifting frequency
			mHalfCycle.freq = Frequency::F1;
			strict_f = Frequency::F12;
		}
		else {
			mHalfCycle.freq = Frequency::UndefinedFrequency;
			strict_f = Frequency::UndefinedFrequency;
		}
	}
	else {
		mHalfCycle.freq = Frequency::UndefinedFrequency;
		strict_f = Frequency::UndefinedFrequency;
	}

	if (true || half_cycle.phaseShift != mHalfCycle.phaseShift)
		DEBUG_PRINT(
			getTime(), DBG, "%s (%d:%s) => %s/%s (%d:%s): Phase shifted from %d to %d degrees\n",
			_FREQUENCY(half_cycle.freq), half_cycle.duration, _LEVEL(half_cycle.level),
			_FREQUENCY(mHalfCycle.freq), _FREQUENCY(strict_f), mHalfCycle.duration, _LEVEL(mHalfCycle.level),
			half_cycle.phaseShift, mHalfCycle.phaseShift
		);
}

//
// Check for valid range for either an F1 or an F2 1/2 cycle
//
// The valid range for an 1/2 cycle extends to the threshold
// between an F1 & F2 1/2 cycle.
// 
// The valid range for an F1 (long) 1/2 cycle to clearly distinguish it from an F2 1/2 cycle
// is [F1/F2 1/2 cycle threshold, max F1 1/2 cycle]
// 
// The valid range for an F2 (short) 1/2 cycle to clearly distinguish it from an F1 1/2 cycle
// is [min F2 1/2 cycle, F1/F2 1/2 cycle threshold]
// 
bool CycleDecoder::validHalfCycleRange(Frequency f, int duration)
{
	if (f == Frequency::F2)
		return (duration >= mCT.mMinNSamplesF2HalfCycle && (double)duration <= mCT.mSamplesThresholdHalfCycle);
	else if (f == Frequency::F1)
		return ((double) duration > mCT.mSamplesThresholdHalfCycle && duration <= mCT.mMaxNSamplesF1HalfCycle);
	else if (f == Frequency::F12)
		return (duration >= mCT.mMinNSamplesF12HalfCycle && duration <= mCT.mMaxNSamplesF12HalfCycle);
	else
		return false;
}

// Check for valid range for either an F1, F2 or an F12 1/2 cycle
// Only the specified tolerance will be used to validate a 1/2 cycle
// duration.
// 
// The valid range for an F1 (short) 1/2 cycle is [min F1 1/2 cycle, max F1 1/2 cycle]
//
// The valid range for an F2 (short) 1/2 cycle is [min F2 1/2 cycle, max F2 1/2 cycle]
//
bool CycleDecoder::strictValidHalfCycleRange(Frequency f, int duration)
{
	if (f == Frequency::F2)
		return (duration >= mCT.mMinNSamplesF2HalfCycle && duration <= mCT.mMaxNSamplesF2HalfCycle);
	else if (f == Frequency::F1)
		return (duration > mCT.mMinNSamplesF1HalfCycle && duration <= mCT.mMaxNSamplesF1HalfCycle);
	else if (f == Frequency::F12)
		return (duration >= mCT.mMinNSamplesF12HalfCycle && duration <= mCT.mMaxNSamplesF12HalfCycle);
	else
		return false;
}


// Return carrier frequency [Hz]
double CycleDecoder::carrierFreq()
{
	return mCT.baseFreq * 2;
}

// Return carrier frequency [Hz]
void CycleDecoder::setCarrierFreq(double carrierFreq)
{
	mCT.set(carrierFreq);
}