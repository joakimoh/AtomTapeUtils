#include "../shared/CommonTypes.h"
#include "LevelDecoder.h"
#include "../shared/WaveSampleTypes.h"
#include <iostream>
#include "../shared/Debug.h"

using namespace std;


// Save the current file position
bool LevelDecoder::checkpoint()
{
	CheckPointSample checkpoint_sample = { mSamplesIndex, mState };
	mCheckPoint.push_back(checkpoint_sample);
	return true;
}

// Roll back to a previously saved file position
bool LevelDecoder::rollback()
{
	CheckPointSample checkpoint_sample;
	checkpoint_sample = mCheckPoint.back();
	mCheckPoint.pop_back();
	mState = checkpoint_sample.state;
	mSamplesIndex = checkpoint_sample.index;
	return true;
}

LevelDecoder::LevelDecoder(
	int sampleFreq, Samples &samples, double startTime, ArgParser &argParser
): mSamples(samples), mArgParser(argParser) { // A reference can only be initialised this way!

	mTracing = argParser.tracing;
	mHighThreshold = (int) round(mArgParser.levelThreshold * SAMPLE_HIGH_MAX);
	mLowThreshold = (int) round(mArgParser.levelThreshold * SAMPLE_LOW_MIN);
	
	mSamples = samples;
	mSamplesIndex = 0;

	mFS = sampleFreq;
	mTS = 1 / mFS;

	// A phase should never be longer than the max value of half an F1 cycle
	mNLevelSamplesMax = (int) round((1 + mArgParser.freqThreshold) * mFS / (F1_FREQ * 2)); 

	// Advance to time startTime before searching for data
	if (startTime > 0)
		while (mSamplesIndex < mSamples.size() && (mSamplesIndex * mTS < startTime)) mSamplesIndex++;

	
}


bool LevelDecoder::getNextSample(Level& level, int& sampleNo) {

	if (mSamplesIndex == mSamples.size())
		return false;

	sampleNo = mSamplesIndex;
	Sample sample = mSamples[mSamplesIndex++];

	if ((mState == NoCarrierLevel || mState == LowLevel) && sample > mHighThreshold) {
		mState = HighLevel;
		mNSamplesLowLevel = 0;
	}
	else if ((mState == NoCarrierLevel || mState == HighLevel) && sample < mLowThreshold) {
		mState = LowLevel;
		mNSamplesHighLevel = 0;
	}
	else if (
			(mState == LowLevel && mNSamplesLowLevel > mNLevelSamplesMax) ||
			(mState == HighLevel && mNSamplesHighLevel > mNLevelSamplesMax)
		) {
		mState = NoCarrierLevel;
		mNSamplesLowLevel = 0;
		mNSamplesHighLevel = 0;
	} else if (mState == LowLevel) { // Unchanged level => measure time staying at same level
		mNSamplesLowLevel++;
	}
	else { // mState == High
		mNSamplesHighLevel++;
	}


	level = mState;

	return true;
}



bool LevelDecoder::endOfSamples() { return (mSamplesIndex == mSamples.size()); }

int LevelDecoder::getSampleNo() { return mSamplesIndex;}

Level LevelDecoder::getLevel() {
	return mState;
}

double LevelDecoder::getTime()
{
	return LevelDecoder::getSampleNo() * mTS;
}
