#pragma once

#ifndef WAVE_SAMPLE_TYPES_H
#define WAVE_SAMPLE_TYPES_H

#include <vector>
#include <cstdint>

using namespace std;

typedef uint8_t ByteSample;
typedef vector<ByteSample> ByteSamples;
typedef vector<ByteSample>::iterator ByteSampleIter;
typedef int16_t Sample;
typedef vector<Sample> Samples;
typedef vector<Sample>::iterator SampleIter;

const Sample	SAMPLE_HIGH_MAX = 32767;
const Sample	SAMPLE_LOW_MIN = -32768;
const Sample	SAMPLE_UNDEFINED = 0;

const int		F1_FREQ = 1200; // low FSK frequency
const int		F2_FREQ = 2400; // high FSK frequency

enum Bit { LowBit, HighBit, UndefinedBit };

typedef struct {
	Bit bit;
	int sampleStart, sampleEnd;
} BitSample;

typedef vector<BitSample> BitSamples;
typedef BitSamples::iterator BitSamplesIter;

enum Level { LowLevel, HighLevel, NoCarrierLevel };

enum Frequency { F1, F2, F12, NoCarrierFrequency, UndefinedFrequency };


#define _BIT(x) (x==BlockDecoder::Bit::Low?"Low":(x==BlockDecoder::Bit::High?"High":"Undefined"))
#define _HEX_DIGIT(x) (string() + (x >= 0 && x < 10?(char) ((int) '0' + x): (char) ((int) 'a' + (x & 0xf) - 10)))
#define _BYTE(x) (_HEX_DIGIT(((x >> 4) & 0xf)) + _HEX_DIGIT((x & 0xf)))
#define _LEVEL(x) (x==Level::LowLevel?"Low":(x==Level::HighLevel?"High":"NoCarrier"))
#define _FREQUENCY(x) (x==Frequency::F1?"F1":(x==Frequency::F2?"F2":(x==Frequency::NoCarrierFrequency?"NoCarrier":(x==Frequency::F12?"F12":"Undefined"))))
#define _HALF_CYCLE(x) (x == HighHalfCycle ? "High" : "Low")

#define _BIT_SAMPLE(x) (x==BlockDecoder::Bit::Low?SAMPLE_LOW_MIN:(x==BlockDecoder::Bit::High?SAMPLE_HIGH_MAX:SAMPLE_UNDEFINED))
#define _LEVEL_SAMPLE(x) (x==Level::Low?SAMPLE_LOW_MIN:(x==Level::High?SAMPLE_HIGH_MAX:SAMPLE_UNDEFINED))
#define _CYCLE_SAMPLE(x) (x==Frequency::F1?SAMPLE_LOW_MIN:(x==Frequency::F2?SAMPLE_HIGH_MAX:SAMPLE_UNDEFINED))




#endif
