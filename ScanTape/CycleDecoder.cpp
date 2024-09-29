#include "CycleDecoder.h"
#include <iostream>
#include <cmath>

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

	cout << "Valid F12 cycles: [" << this->mMinNSamplesF12Cycle << ", " << n_samples_F12 << ", " <<
		this->mMaxNSamplesF12Cycle << "]\n";

	cout << "Valid F1 cycles: [" << this->mMinNSamplesF1Cycle << ", " << n_samples_F1 << ", " <<
		this->mMaxNSamplesF1Cycle << "]\n";

	cout << "Valid F2 cycles: [" << this->mMinNSamplesF2Cycle << ", " << n_samples_F2 << ", " <<
		this->mMaxNSamplesF2Cycle << "]\n";
	
	cout << "Valid F1 1/2 cycles: [" << this->mMinNSamplesF1HalfCycle << " (" << this->mSamplesThresholdHalfCycle << ")" << 
		", " << n_samples_F1 / 2 << ", " << this->mMaxNSamplesF1HalfCycle <<  "]\n";

	cout << "Valid F2 1/2 cycles: [" << this->mMinNSamplesF2HalfCycle << ", " << n_samples_F2 / 2 << ", " <<
		this->mMaxNSamplesF2HalfCycle << " (" << this->mSamplesThresholdHalfCycle << ")" << "]\n";


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

	// Min & max durations of F1 & F2 frequency low/high 1/2 cycles
	mMinNSamplesF1Cycle = (int)round((1 - freqThreshold) * fS / f1); // Min duration of an F1 cycle
	mMaxNSamplesF1Cycle = (int)round((1 + freqThreshold) * fS / f1); // Max duration of an F1 cycle
	
	mMinNSamplesF2Cycle = (int)round((1 - freqThreshold) * fS / f2); // Min duration of an F2 cycle
	mMaxNSamplesF2Cycle = (int)round((1 + freqThreshold) * fS / f2); // Max duration of an F2 cycle
	
	mMinNSamplesF12Cycle = (int)round((1 - freqThreshold) * 3 * fS / (f2 * 2));// Min duration of a 3T/2 cycle where T = 1/F2	
	mMaxNSamplesF12Cycle = (int)round((1 + freqThreshold) * 3 * fS / (f2 * 2)); // Min duration of a 3T/2 cycle where T = 1/F2

	mMinNSamplesF1HalfCycle = (int)round((1 - freqThreshold) * fS / (f1 * 2));
	mMaxNSamplesF1HalfCycle = (int)round((1 + freqThreshold) * fS / (f1 * 2));
	mMinNSamplesF2HalfCycle = (int)round((1 - freqThreshold) * fS / (f2 * 2));
	mMaxNSamplesF2HalfCycle = (int)round((1 + freqThreshold) * fS / (f2 * 2));

	mSamplesThresholdHalfCycle = ((double) fS / f1 + (double) fS / f2) / 4;


}

CycleDecoder::CycleDecoder(int sampleFreq, ArgParser &argParser) : mArgParser(argParser), mTracing(argParser.tracing),
mVerbose(argParser.verbose)
{
	mCT.set(sampleFreq, F2_FREQ, mArgParser.freqThreshold);
	
}

// Record the frequency of the last 1/2 cycle (but only if a 1/2 cycle was detected)
void CycleDecoder::updateHalfCycleFreq(int half_cycle_duration, Frequency& lastHalfCycleFrequency)
{
	if (half_cycle_duration >= mCT.mMinNSamplesF2HalfCycle && (double) half_cycle_duration <= mCT.mSamplesThresholdHalfCycle)
		lastHalfCycleFrequency = Frequency::F2;
	else if ((double) half_cycle_duration > mCT.mSamplesThresholdHalfCycle && half_cycle_duration <= mCT.mMaxNSamplesF1HalfCycle)
		lastHalfCycleFrequency = Frequency::F1;
	else if (half_cycle_duration > 0)
		lastHalfCycleFrequency = Frequency::UndefinedFrequency;
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
		return (duration >= mCT.mMinNSamplesF2HalfCycle && (double) duration <= mCT.mSamplesThresholdHalfCycle);
	else  // f == Frequency::F1
		return (duration > mCT.mSamplesThresholdHalfCycle && duration <= mCT.mMaxNSamplesF1HalfCycle);
}

// Check for valid range for either an F1 or an F2 1/2 cycle
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
	else  // f == Frequency::F1
		return (duration > mCT.mMinNSamplesF1HalfCycle && duration <= mCT.mMaxNSamplesF1HalfCycle);
}

// Collect a specified no of cycles of a certain frequency
bool CycleDecoder::collectCycles(
	Frequency freq, int nRequiredCycles,
	CycleSample& lastValidCycleSample, int& nCollectedCycles
) {

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
bool CycleDecoder::collectCycles(Frequency freq, CycleSample& lastValidCycleSample, int maxCycles, int& nCollectedCycles) {

	CycleSample sample;
	nCollectedCycles = 0;
	while (getNextCycle(sample) && sample.freq == freq && nCollectedCycles < maxCycles) {
		lastValidCycleSample = sample;
		nCollectedCycles++;
	}


	return true;
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