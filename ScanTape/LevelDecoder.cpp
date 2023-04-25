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
	Samples &samples, double startTime, ArgParser &argParser
): mSamples(samples), mArgParser(argParser) { // A reference can only be initialised this way!

	mTracing = argParser.tracing;
	mHighThreshold = (int) round(mArgParser.mLevelThreshold * SAMPLE_HIGH_MAX);
	mLowThreshold = (int) round(mArgParser.mLevelThreshold * SAMPLE_LOW_MIN);
	
	mSamples = samples;
	mSamplesIndex = 0;

	// A phase should never be longer than the max value of half an F1 cycle
	mNLevelSamplesMax = (int) round((1 + mArgParser.mFreqThreshold) * F_S / (F1_FREQ * 2)); 

	// Advance to time startTime before searching for data
	if (startTime > 0)
		while (mSamplesIndex < mSamples.size() && (mSamplesIndex * T_S < startTime)) mSamplesIndex++;

	
}


bool LevelDecoder::getNextSample(Level& level, int& sampleNo) {

	if (mSamplesIndex == mSamples.size())
		return false;

	sampleNo = mSamplesIndex;
	Sample sample = mSamples[mSamplesIndex++];

	if ((mState == NoCarrier || mState == Low) && sample > mHighThreshold) {
		mState = High;
		mNSamplesLowLevel = 0;
	}
	else if ((mState == NoCarrier || mState == High) && sample < mLowThreshold) {
		mState = Low;
		mNSamplesHighLevel = 0;
	}
	else if (
			(mState == Low && mNSamplesLowLevel > mNLevelSamplesMax) ||
			(mState == High && mNSamplesHighLevel > mNLevelSamplesMax)
		) {
		mState = NoCarrier;
		mNSamplesLowLevel = 0;
		mNSamplesHighLevel = 0;
	} else if (mState == Low) { // Unchanged level => measure time staying at same level
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

LevelDecoder::Level LevelDecoder::getLevel() {
	return mState;
}

double LevelDecoder::getTime()
{
	return LevelDecoder::getSampleNo() * T_S;
}
