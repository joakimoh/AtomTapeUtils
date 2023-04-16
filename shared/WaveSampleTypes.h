#pragma once

#ifndef WAVE_SAMPLE_TYPES_H
#define WAVE_SAMPLE_TYPES_H

#include <vector>

using namespace std;

typedef int16_t Sample;
typedef vector<Sample> Samples;
typedef vector<Sample>::iterator SampleIter;

const Sample	SAMPLE_HIGH_MAX = 32767;
const Sample	SAMPLE_LOW_MIN = -32768;
const Sample	SAMPLE_UNDEFINED = 0;

const int		F_S = 44100; // sample frequency
const double	T_S = 1.0 / (double)F_S;
const int		F1_FREQ = 1200; // low FSK frequency
const int		F2_FREQ = 2400; // high FSK frequency

typedef enum { Low, High, Undefined } Bit;

typedef struct {
	Bit bit;
	int sampleStart, sampleEnd;
} BitSample;

typedef vector<BitSample> BitSamples;
typedef BitSamples::iterator BitSamplesIter;


#define _BIT(x) (x==BlockDecoder::Bit::Low?"Low":(x==BlockDecoder::Bit::High?"High":"Undefined"))
#define _LEVEL(x) (x==LevelDecoder::Level::Low?"Low":(x==LevelDecoder::Level::High?"High":"NoCarrier"))
#define _FREQUENCY(x) (x==CycleDecoder::Frequency::F1?"F1":(x==CycleDecoder::Frequency::F2?"F2":(x==CycleDecoder::Frequency::NoCarrier?"NoCarrier":"Undefined")))

#define _BIT_SAMPLE(x) (x==BlockDecoder::Bit::Low?SAMPLE_LOW_MIN:(x==BlockDecoder::Bit::High?SAMPLE_HIGH_MAX:SAMPLE_UNDEFINED))
#define _LEVEL_SAMPLE(x) (x==LevelDecoder::Level::Low?SAMPLE_LOW_MIN:(x==LevelDecoder::Level::High?SAMPLE_HIGH_MAX:SAMPLE_UNDEFINED))
#define _CYCLE_SAMPLE(x) (x==CycleDecoder::Frequency::F1?SAMPLE_LOW_MIN:(x==CycleDecoder::Frequency::F2?SAMPLE_HIGH_MAX:SAMPLE_UNDEFINED))




#endif
