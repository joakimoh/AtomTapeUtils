
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include "PcmFile.h"
#include <sstream>
#include <iomanip>

using namespace std;

string str4(char c[4])
{
    stringstream ss;
    for (int i = 0; i < 4; i++)
        if (c[i] >= 0x20 && c[i] <= 0x7e)
            ss <<  c[i];
        else
            ss << "\\" << std::setfill('0') << std::setw(2) << hex << int(c[i]);

    return ss.str();
}

bool readSamples(string fileName, Samples &samples, int& sampleFreq)
{

    ifstream fin(fileName, ios::in | ios::binary | ios::ate);

    if (!fin) {
        cout << "couldn't open file " << fileName << "\n";
        return false;
    }

    auto fin_sz = fin.tellg();


    
    
    CommonHeader h_head;
    
    fin.seekg(0);

    // Read first part of WAV header
    fin.read((char*)&h_head, sizeof(h_head));

    // CheckType of format - should be 1 for PCM
    if (h_head.audioFormat != 1 /* PCM */) {
        cout << "Input file has no data or is not a valid one channel 16 - bit PCM Wave file!\n";
        fin.close();
        return false;
    }

    // Read last part of WAV header
    HeaderTail h_tail;
    fin.read((char*)&h_tail, sizeof(h_tail));


    // Check sub chunk size
    if (h_tail.subchunk2Size != h_head.ChunkSize - 36) {
        cout << "Size of data samples (subchunk2Size = " << h_tail.subchunk2Size <<
            ") not consistent with file size (ChunkSize + 8 = " << (h_head.ChunkSize + 8) << ")!\n";
        
        // correct the chunk sizes before attempting to read the samples... 
        h_head.ChunkSize = (uint32_t) fin_sz - 8;
        h_tail.subchunk2Size = h_head.ChunkSize - 36;

        cout << "Recalculating the size of the data samples to " << h_head.ChunkSize << " bytes...\n";
    }

    // Check that the there is only one channel with 16-bit samples and a sample frequency of 44.1 kHz
    if (!(
        h_head.numChannels == 1 &&
        h_head.bitsPerSample == 16 /* 16b-bit */

        )
        ) {
        cout << "Input file is not an one channel 16 - bit PCM Wave file:\n";
        cout << "format: " << h_head.audioFormat << " (1 <=> PCM)\n";
        cout << "#channels: " << h_head.numChannels << " (1)\n";
        cout << "sample rate: " << h_head.sampleRate << " (44 100) \n";
        cout << "sample size: " << h_head.bitsPerSample << " (16)\n";

        fin.close();

        return false;
    }

    sampleFreq = h_head.sampleRate;

    // Collect all samples into a vector 'samples'
    samples = Samples(h_tail.subchunk2Size / 2);
    Sample* samples_p = &samples.front();
    fin.read((char*)samples_p, (streamsize) h_tail.subchunk2Size);
    fin.close();

    return true;
}

bool writeSamples(string fileName, Samples samples[], const int nChannels, int sampleFreq)
{
    // Check that each channel contains the same no of samples
    int n_samples = (int) samples[0].size();
    for (int i = 1; i < nChannels; i++) {
        if (samples[i].size() != n_samples) {
            cout << "Channel #" << i << " have different no of samples (" << samples[i].size() << ") than channel #0 (" << n_samples << ")!\n";
            return false;
        }
    }


    ofstream fout(fileName, ios::out | ios::binary | ios::ate);
    if (!fout) {
        cout << "can't write to WAV file " << fileName << "\n";
        return false;
    }

    CommonHeader h_head;

    strncpy(h_head.chunkId,"RIFF", 4);
    strncpy(h_head.format, "WAVE", 4);
    strncpy(h_head.subchunk1ID, "fmt ", 4);
    h_head.ChunkSize = 36 + nChannels * n_samples * nChannels * 2; // 36 + subChunk2Size = 36 + NumSamples * NumChannels * 2
    h_head.numChannels = nChannels;
    h_head.byteRate = sampleFreq * nChannels * 2; //SampleRate * NumChannels * 2
    h_head.blockAlign = nChannels * 2; // NumChannels * 2

    HeaderTail h_tail;
    strncpy(h_tail.subchunk2ID, "data", 4);
    h_tail.subchunk2Size = n_samples * nChannels * 2; // NumSamples * NumChannels * 2

    // Write header + data chunk header
    fout.write((char*)&h_head, sizeof(h_head));
    fout.write((char*)&h_tail, sizeof(h_tail));

    // Create one sample iterator per channel
    vector<SampleIter> sample_iter;
    for (int c = 0; c < nChannels; c++) {
        sample_iter.push_back(samples[c].begin());
    }

    // Iterate over all samples, picking one sample per channel at a time,
    // and write it to PCM output file.
    if (nChannels == 1) {
        // Optimise for speed when there is only one channel to write
        Sample* samples_p = &samples[0].front();
        fout.write((char*) samples_p, (streamsize)h_tail.subchunk2Size);
    }
    else {
        int sample_sz = sizeof(Sample);
        for (int s = 0; s < n_samples; s++) {
            for (int c = 0; c < nChannels; c++) {
                //Sample sample = *sample_iter[c]++;
                fout.write((char*)&samples[c][s], sample_sz);
            }
        }
    }
    
    return true;

}

