#ifndef PCM_FILE_H
#define PCM_FILE_H

#include <cstdint>
#include "WaveSampleTypes.h"
#include "Logging.h"



using namespace std;




typedef struct CommonHeader_struct {
    char chunkId[4]; // "RIFF"
    uint32_t ChunkSize; // size of the rest of the chunk = sizeof(CommonHeader) + subChunk2Size = 36 + subChunk2Size = filesize - 8
    char format[4]; // "WAVE"
    char subchunk1ID[4];// "fmt\0"
    uint32_t subchunk1Size = 16; // For PCM
    uint16_t audioFormat = 1; // PCM
    uint16_t numChannels = 1;
    uint32_t sampleRate = 44100;
    uint32_t byteRate; // SampleRate * NumChannels * BitsPerSample/8
    uint16_t blockAlign; //  NumChannels * BitsPerSample/8
    uint16_t bitsPerSample = 16;
    // 8-bit samples are stored as unsigned bytes, ranging from 0 to 255.
    // 16-bit samples are stored as 2's-complement signed integers, ranging from -32768 to 32767
} CommonHeader;

typedef struct HeaderTail_struct {
    char subchunk2ID[4]; // "data"
    uint32_t subchunk2Size; // NumSamples * NumChannels * BitsPerSample/8

} HeaderTail;

class PcmFile {

private:

    static string str4(char c[4]);

public:

    // Read samples from a one channel 16-bit 44.1 kHz PCM WAW file
    static bool readSamples(string fileName, Samples* &samples, int& sampleFreq, Logging logging);


    // Write sample vector into a multiple channel 16-bit 44.1 kHz PCM WAW file
    static bool writeSamples(string fileName, Samples *samples[], int nChannels, int sampleFreq, Logging logging);


};

#endif // !PCM_FILE_H
