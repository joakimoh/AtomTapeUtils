#pragma once

#ifndef LEVEL_DECODER_H
#define LEVEL_DECODER_H

#include "../shared/WaveSampleTypes.h"
#include "ArgParser.h"


class LevelDecoder {

public:
	

	typedef vector<Level> Levels;
	typedef vector<Level>::iterator LevelIter;

private:

	int mFS; // sample frequency (normally 44 100 Hz for WAV files)
	double mTS = 1 / mFS; // sample duration = 1 / sample frequency

	int mNLevelSamplesMax;

	ArgParser mArgParser;

	Samples& mSamples;
	
	bool mTracing;
	
	Sample mHighThreshold;
	Sample mLowThreshold;

	

	class LevelInfo
	{
	public:
		int nSamplesLow = 0;
		int nSamplesHigh = 0;
		int sampleIndex = 0;
		Level state = NoCarrierLevel;
	} ;

	LevelInfo mLevelInfo = { 0, 0, 0, NoCarrierLevel };

	// Checkpoint data for rollback
	//int mSamplesIndexCheckpoint;
	//Level mStateCheckpoint;

	vector<LevelInfo> mCheckPoints;
	

public:

	LevelDecoder(int sampleFreq, Samples& samples, double startTime, ArgParser &argParser);

	bool getNextSample(Level &level, int &sampleNo);

	Level getLevel();

	bool endOfSamples();

	int getSampleNo();

	double getTime();

	// Save the current file position
	bool checkpoint();

	// Roll back to a previously saved file position
	bool rollback();

	// Remove checkpoint (without rolling back)
	bool regretCheckpoint();
};

#endif