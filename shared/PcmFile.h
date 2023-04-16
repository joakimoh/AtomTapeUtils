#ifndef PCM_FILE_H
#define PCM_FILE_H

#include "WaveSampleTypes.h"



using namespace std;




typedef struct CommonHeader_struct {
    char chunkId[4]; // "RIFF"
    uint32_t ChunkSize; // sizeof(CommonHeader) + subChunk2Size = 36 + subChunk2Size = filesize - 8
    char format[4]; // "WAVE"
    char subchunk1ID[4];// "fmt\0"
    uint32_t subchunk1Size = 16; // For PCM
    uint16_t audioFormat = 1; // PCM
    uint16_t numChannels = 1;
    uint32_t sampleRate = 44100;
    uint32_t byteRate; // SampleRate * NumChannels * BitsPerSample/8
    uint16_t blockAlign; //  NumChannels * BitsPerSample/8
    uint16_t bitsPerSample = 16;
} CommonHeader;

typedef struct HeaderTail_struct {
    char subchunk2ID[4]; // "data"
    uint32_t subchunk2Size; // NumSamples * NumChannels * BitsPerSample/8

} HeaderTail;


// Read samples from a one channel 16-bit 44.1 kHz PCM WAW file
bool readSamples(string fileName, Samples &samples);


// Write sample vector into a multiple channel 16-bit 44.1 kHz PCM WAW file
bool writeSamples(string fileName, Samples samples[], int nChannels);




#endif // !PCM_FILE_H