#pragma once

#ifndef LEVEL_DECODER_H
#define LEVEL_DECODER_H

#include "../shared/WaveSampleTypes.h"
#include "ArgParser.h"


class LevelDecoder {

public:
	typedef enum { Low, High, NoCarrier } Level;

	typedef vector<Level> Levels;
	typedef vector<Level>::iterator LevelIter;

private:

	int mNLevelSamplesMax;

	ArgParser mArgParser;

	Samples& mSamples;
	int mSamplesIndex;

	bool mTracing;
	

	Level mState = NoCarrier;
	

	Sample mHighThreshold;
	Sample mLowThreshold;

	int mNSamplesLowLevel = 0;
	int mNSamplesHighLevel = 0;


	// Checkpoint data for rollback
	int mSamplesIndexCheckpoint;
	Level mStateCheckpoint;

public:

	LevelDecoder(Samples& samples, double startTime, ArgParser &argParser);

	bool getNextSample(Level &level, int &sampleNo);

	Level getLevel();

	bool endOfSamples();

	int getSampleNo();

	double getTime();

	// Save the current file position
	bool checkpoint();

	// Roll back to a previously saved file position
	bool rollback();
};

#endif