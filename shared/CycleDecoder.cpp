#include "CycleDecoder.h"
#include <iostream>
#include <cmath>
#include "Utility.h"
#include "Logging.h"
#include <sstream>

string HalfCycleInfo::info()
{
	ostringstream s;
	s << _FREQUENCY(this->freq) << " (" << _LEVEL(this->level) << ":" <<
		this->duration << ":" << this->phaseShift << ")";

	return s.str();
}

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

	cout << "Valid F12 1/2 cycles: [" << this->mMinNSamplesF12HalfCycle << ", " << n_samples_F12 / 2 << ", " <<
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
	double f = 1 + freqThreshold;
	double f_min = 1 - freqThreshold;
	double f_max = 1 + freqThreshold;


	mMinNSamplesF1HalfCycle = f_min * fS / (f1 * 2); // Min duration of an F1 1/2 cycle
	mMaxNSamplesF1HalfCycle = f_max * fS / (f1 * 2); // Max duration of an F1 1/2 cycle
	mMinNSamplesF2HalfCycle = f_min * fS / (f2 * 2); // Min duration of an F2 1/2 cycle
	mMaxNSamplesF2HalfCycle = f_max * fS / (f2 * 2); // Max duration of an F2 1/2 cycle

	// Define an F12 1/2 cycle as the interval between valid F1 and F2 1/2 cycles
	//mMinNSamplesF12HalfCycle = mMaxNSamplesF2HalfCycle;
	//mMaxNSamplesF12HalfCycle = mMinNSamplesF1HalfCycle;

	mMinNSamplesF12HalfCycle = f_min * 3 * fS / (f2 * 4);// Min duration of a 3T/4 1/2 cycle where T = 1/F2	
	mMaxNSamplesF12HalfCycle = f_max * 3 * fS / (f2 * 4); // Min duration of a 3T/4 1/2 cycle where T = 1/F2


	//mSamplesThresholdHalfCycle = ((double) fS / f1 + (double) fS / f2) / 4;
	mSamplesThresholdHalfCycle = (mMaxNSamplesF2HalfCycle + mMinNSamplesF1HalfCycle) / 2;

}

CycleDecoder::CycleDecoder(int sampleFreq, double freqThreshold, Logging logging):
	mDebugInfo(logging)
{
	mCT.set(sampleFreq, F2_FREQ, freqThreshold);

	if (mDebugInfo.verbose) {
		cout << "\n\nCycle Sample Timing:\n\n";
		mCT.log();
		cout << "\n\n";
	}
	
}

//
// Get phase shift when a frequency shift occurs.
// Only checked for when transitioning from different types of 1/2 cycles.
// 
// F12 <=> 1/2 of duration between the F2 and F2 1/2 cycle duration ranges
//
// 1/2 Cycle sequence			Phase determination
// F2 + F1						low F1 => 180 degrees, high => 0 degrees
// (F2 +) F12 + F1				low F1 => 90 degrees, high => 270 degrees
// F1 + F2						low F2 => 180 degrees, high = 0 degrees
// (F1 +) F12 + F2				low F2 => 90 degrees, high => 270 degrees
// 
void CycleDecoder::updatePhase(Frequency f1, Frequency f2, Level f2Level)
{
	if (f1 == f2 || f2 == Frequency::UndefinedFrequency || f2 == Frequency::NoCarrierFrequency)
		return;

	if (f2Level == Level::HighLevel) {
		if (f1 == Frequency::F2 && f2 == Frequency::F1)
			mHalfCycle.phaseShift = 0;
		else if (f1 == Frequency::F12 && f2 == Frequency::F1)
			mHalfCycle.phaseShift = 270;
		else if (f1 == Frequency::F1 && f2 == Frequency::F2)
			mHalfCycle.phaseShift = 0;
		else if (f1 == Frequency::F12 && f2 == Frequency::F2)
			mHalfCycle.phaseShift = 270;
	}
	else if (f2Level == Level::LowLevel) {
		if (f1 == Frequency::F2 && f2 == Frequency::F1)
			mHalfCycle.phaseShift = 180;
		else if (f1 == Frequency::F12 && f2 == Frequency::F1)
			mHalfCycle.phaseShift = 90;
		else if (f1 == Frequency::F1 && f2 == Frequency::F2)
			mHalfCycle.phaseShift = 180;
		else if (f1 == Frequency::F12 && f2 == Frequency::F2)
			mHalfCycle.phaseShift = 90;
	}
}

//
// Record the frequency and phase of the last 1/2 cycle (considering the preceeding 1/2 cycle)
// 
// The classification is based on the patterns below:
// 
// 1/2 Cycle sequence			Phase determination									Data bit determination*	#transitions
// F2 +	2n x F1 +	F2			low first F1 => 180 degrees, high = 0 degrees		'0'						[2n-1,2n+1]
// F12 +	2n-1 x F1 +	F12		low first F1 => 90 degrees, high => 270 degrees		'0'						2n
// F1 +	4n x F2 +	F1			low first F2 => 180 degrees, high = 0 degrees		'1'						[4n-1,4n+1]
// F12 +	4n-1 x F2 +	F12		low first F2 => 90 degrees, high => 270 degrees		'1'						4n
// 
// n = 1 for 1200 baud and 4 for 300 baud
// The first 1/2 cycle is the preceeding one and it's the second 1/2 cycle that is the one currently being evaluated.
// The remaining 1/2 cycles are shown only to show also the connection to a data bit. The no of transitions shown
// are for a time window (of a duration corresponding to one data bit). For 1200 baud the no of transitions for a '0' [1,3]
// will overlap with the no of transitions for a '1' [3,5] which makes it difficult to use the no of transitions alone
// to determine the value of a data bit.(For 300 baud this intervals [7,9] for '0' and [15,17] for '1' will not overlap and
// the no of transitions woul work well for determining the value of a data bit by the use of a threshold of (8+16)/2=12.
// to distinguish between a '0' and a '1'.) Still, you would like to use the no of transitions in some way as it can
// tolerate some faults impacting the data bits (at least at 300 baud).
// 
// An F12 1/2 cycle is classified either as an F1 or F2 cycle. If it follows upon an F2 1/2 cycle it
// is classified as an F1 1/2 cycle (assuming it would be the end of a carrier and the beginning of a start bit).
// Otherwise it is classified as an F1 1/2 cycle.
// 
// * The criteria used to determine the data bit value as '0' (see getDataBit() in WavTapeReader) is currently:
//		"No of detected 1/2 cycles for a duration corresponding to one data bit" <= threshold between '0' and '1' &&
//		max 1/2 cycle duration is a valid F1 1/2 cycle
//	 Otherwise it will be classified as '1'.
// 
// Neither the recorded phase nor the sequences above are currently used to determine the value of a data bit.
//
void CycleDecoder::updateHalfCycleFreq(int halfCycleDuration, Level halfCycleLevel)
{
	// Save information about previous 1/2 cycle
	HalfCycleInfo hc_p = mHalfCycle;

	// Record level and duration
	mHalfCycle.level = halfCycleLevel;
	mHalfCycle.duration = halfCycleDuration;

	// Classify previous and current 1/2 cycle as either F12 or F1/F2 (F12 having lowest priority)
	Frequency fr_p = getHalfCycleFrequency(hc_p.duration);
	Frequency fr = getHalfCycleFrequency(mHalfCycle.duration);

	// Determine phaseshift
	updatePhase(fr_p, fr, halfCycleLevel);

	// Determine frequency (F1, F2 or invalid)
	if (fr == Frequency::F1 || fr == Frequency::F2)
		mHalfCycle.freq = fr;
	else if (fr == Frequency::F12) {
		if (hc_p.freq == Frequency::F1) // One F1 1/2 cycle followed by one F12 1/2 cycle. Treat this as an F2 cycle.
			mHalfCycle.freq = Frequency::F2;
		else if (hc_p.freq == Frequency::F2)  // One F2 1/2 cycle followed by one F12 1/2 cycle. Treat this as an F1 cycle.
			mHalfCycle.freq = Frequency::F1;
		else
			mHalfCycle.freq = Frequency::UndefinedFrequency;
	}
	else
		mHalfCycle.freq = Frequency::UndefinedFrequency;


	if (true || hc_p.phaseShift != mHalfCycle.phaseShift)
		DEBUG_PRINT(
			getTime(), DBG,
			"%s/%s => %s/%s\n",
			hc_p.info().c_str(), _FREQUENCY(fr_p), mHalfCycle.info().c_str(), _FREQUENCY(fr)
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

// Determine the type of 1/2 cycle (F1, F2, F12 or unknown) - F12 has highest priority
Frequency CycleDecoder::getHalfCycleFrequencyRange(int duration)
{
	if (duration >= mCT.mMinNSamplesF12HalfCycle && duration <= mCT.mMaxNSamplesF12HalfCycle)
		return Frequency::F12;
	else if (duration >= mCT.mMinNSamplesF1HalfCycle && duration <= mCT.mMaxNSamplesF1HalfCycle)
		return Frequency::F1;
	else if (duration >= mCT.mMinNSamplesF2HalfCycle && duration <= mCT.mMaxNSamplesF2HalfCycle)
		return Frequency::F2;
	else
		return Frequency::UndefinedFrequency;
}

// Determine the type of 1/2 cycle (F1, F2, F12 or unknown) - F12 has lowest priority
Frequency CycleDecoder::getHalfCycleFrequency(int duration)
{
	if (duration >= mCT.mMinNSamplesF1HalfCycle && duration <= mCT.mMaxNSamplesF1HalfCycle)
		return Frequency::F1;
	else if (duration >= mCT.mMinNSamplesF2HalfCycle && duration <= mCT.mMaxNSamplesF2HalfCycle)
		return Frequency::F2;
	else if (duration >= mCT.mMinNSamplesF12HalfCycle && duration <= mCT.mMaxNSamplesF12HalfCycle)
		return Frequency::F12;
	else
		return Frequency::UndefinedFrequency;
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