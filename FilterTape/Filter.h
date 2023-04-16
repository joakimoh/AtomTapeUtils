#ifndef FILTER_H
#define FILTER_H


#include "../shared/WaveSampleTypes.h"
#include "ArgParser.h"

typedef enum { LOCAL_MAX, LOCAL_MIN, PLATEAU, START_POS_SLOPE, START_NEG_SLOPE } Extremum;

typedef struct {
	Extremum extremum;
	int pos;
} ExtremumSample;

typedef vector<ExtremumSample> ExtremumSamples;
typedef ExtremumSamples::iterator ExtremumSamplesIter;

#define _EXTREMUM(e) (e == LOCAL_MAX? "LOCAL_MAX": (e == LOCAL_MIN?"LOCAL_MIN":(e==PLATEAU?"PLATEAU":(e==START_POS_SLOPE?"START_POS_SLOPE":(e==START_NEG_SLOPE?"START_NEG_SLOPE":"???")))))

class Filter {

public:

	Filter(ArgParser argParser);

	bool averageFilter(Samples& inSamples, Samples& outSamples);

	bool normaliseFilter(Samples& inSamples, ExtremumSamples& outSamples, int& nOutSamples);

	bool plotFromExtremums(int nExtremums, ExtremumSamples& extremums, Samples& newShapes, int nSamples);


private:

	int mSaturationLevelLow;
	int mSaturationLevelHigh;
	int mAveragePoints;
	double mDerivativeThreshold;
	int mMaxSampleAmplitude;
	double minPeakDistanceSamples;

	ExtremumSample mExtremumSample;
	ExtremumSample mPrevExtremumSample;

	bool find_extreme(int &pos, Samples& samples, Extremum extremum, Extremum& newExtremum, int& nexExtremumPos);

	int slope(double i);

	bool plotSinus(double a1, double a2, int nSamples, Samples& samples, int &sampleIndex);

	bool derivative(int pos, Samples& samples, int nSamples, double& d);

	void plotDebug(int debugLevel, ExtremumSample& prevSample, ExtremumSample& sample, int extremumIndex, ExtremumSamples& samples);
	void plotDebug(int debugLevel, string text, ExtremumSample& prevSample, ExtremumSample& sample, int extremumIndex, ExtremumSamples& samples);
	void plotDebug(int debugLevel, string text, ExtremumSample& sample, int extremumIndex, ExtremumSamples& samples);

};

#endif