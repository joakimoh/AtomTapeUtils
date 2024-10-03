#include "../shared/CommonTypes.h"
#include "LevelDecoder.h"
#include "../shared/WaveSampleTypes.h"
#include <iostream>
#include "../shared/Debug.h"
#include <cmath>

using namespace std;


// Save the current file position
bool LevelDecoder::checkpoint()
{

	// Add it to checkpoints
	mCheckPoints.push_back(mLevelInfo);

	return true;
}

// Roll back to a previously saved file position
bool LevelDecoder::rollback()
{
	if (mCheckPoints.size() == 0)
		return false;

	// Get reference to last checkpoint element
	mLevelInfo = mCheckPoints.back();

	// Dispose of the last checkpoint element
	mCheckPoints.pop_back();

	return true;
}

// Remove checkpoint (without rolling back)
bool LevelDecoder::regretCheckpoint()
{
	if (mCheckPoints.size() == 0)
		return false;

	// Dispose of the last checkpoint element
	mCheckPoints.pop_back();

	return true;
}

LevelDecoder::LevelDecoder(
	int sampleFreq, Samples &samples, double startTime, ArgParser &argParser
): mSamples(samples), mArgParser(argParser) { // A reference can only be initialised this way!

	mTracing = argParser.tracing;
	mHighThreshold = (int) round(mArgParser.levelThreshold * SAMPLE_HIGH_MAX);
	mLowThreshold = (int) round(mArgParser.levelThreshold * SAMPLE_LOW_MIN);
	
	mSamples = samples;
	mLevelInfo.sampleIndex = 0;

	mFS = sampleFreq;
	mTS = 1 / mFS;

	// A half_cycle should never be longer than the max value of half an F1 cycle
	mNLevelSamplesMax = (int) round((1 + mArgParser.freqThreshold) * mFS / (F1_FREQ * 2)); 

	// Advance to time startTime before searching for data
	if (startTime > 0)
		while (mLevelInfo.sampleIndex < mSamples.size() && (mLevelInfo.sampleIndex * mTS < startTime)) mLevelInfo.sampleIndex++;

	
}


bool LevelDecoder::getNextSample(Level& level, int& sampleNo) {

	if (mLevelInfo.sampleIndex == mSamples.size())
		return false;

	sampleNo = mLevelInfo.sampleIndex;
	Sample sample = mSamples[mLevelInfo.sampleIndex++];

	if ((mLevelInfo.state == NoCarrierLevel || mLevelInfo.state == LowLevel) && sample > mHighThreshold) {
		mLevelInfo.state = HighLevel;
		mLevelInfo.nSamplesLow = 0;
	}
	else if ((mLevelInfo.state == NoCarrierLevel || mLevelInfo.state == HighLevel) && sample < mLowThreshold) {
		mLevelInfo.state = LowLevel;
		mLevelInfo.nSamplesHigh = 0;
	}
	else if (
			(mLevelInfo.state == LowLevel && mLevelInfo.nSamplesLow > mNLevelSamplesMax) ||
			(mLevelInfo.state == HighLevel && mLevelInfo.nSamplesHigh > mNLevelSamplesMax)
		) {
		mLevelInfo.state = NoCarrierLevel;
		mLevelInfo.nSamplesLow = 0;
		mLevelInfo.nSamplesHigh = 0;
	} else if (mLevelInfo.state == LowLevel) { // Unchanged level => measure time staying at same level
		mLevelInfo.nSamplesLow++;
	}
	else { // mLevelInfo.state == High
		mLevelInfo.nSamplesHigh++;
	}


	level = mLevelInfo.state;

	return true;
}



bool LevelDecoder::endOfSamples() { return (mLevelInfo.sampleIndex == mSamples.size()); }

int LevelDecoder::getSampleNo() { return mLevelInfo.sampleIndex;}

Level LevelDecoder::getLevel() {
	return mLevelInfo.state;
}

double LevelDecoder::getTime()
{
	return LevelDecoder::getSampleNo() * mTS;
}
