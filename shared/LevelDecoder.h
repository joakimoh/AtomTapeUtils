#pragma once

#ifndef LEVEL_DECODER_H
#define LEVEL_DECODER_H

#include "WaveSampleTypes.h"
#include "Logging.h"


class LevelDecoder {

public:
	

	typedef vector<Level> Levels;
	typedef vector<Level>::iterator LevelIter;

private:

	int mFS; // sample frequency (normally 44 100 Hz for WAV files)
	double mTS = 1 / mFS; // sample duration = 1 / sample frequency

	int mNLevelSamplesMax;

	Samples& mSamples;
	
	Logging mDebugInfo;
	
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

	vector<LevelInfo> mCheckPoints;
	

public:

	LevelDecoder(int sampleFreq, Samples& samples, double startTime, double freqThreshold, double levelThreshold, Logging logging);

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