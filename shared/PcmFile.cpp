
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include "PcmFile.h"
#include <sstream>
#include <iomanip>
#include <cstdint>

using namespace std;

string PcmFile::str4(char c[4])
{
    stringstream ss;
    for (int i = 0; i < 4; i++)
        if (c[i] >= 0x20 && c[i] <= 0x7e)
            ss <<  c[i];
        else
            ss << "\\" << std::setfill('0') << std::setw(2) << hex << int(c[i]);

    return ss.str();
}

bool PcmFile::readSamples(string fileName, Samples* &samplesP, int& sampleFreq, Logging logging)
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
    if (h_head.audioFormat != 1 /* PCM */ || !(h_head.bitsPerSample == 16 || h_head.bitsPerSample == 8)) {
        cout << "Input file has no data or is not a valid 8 or 16-bit PCM Wave file!\n";
        fin.close();
        return false;
    }

    // Read last part of WAV header
    HeaderTail h_tail;
    fin.read((char*)&h_tail, sizeof(h_tail));


    // Check sub chunk size
    // ChunkSize = sizeof(CommonHeader) + subChunk2Size = 36 + subChunk2Size = filesize - 8
    if (h_tail.subchunk2Size != h_head.ChunkSize - 36) {
        if (logging.verbose)
            cout << "Size of data samples (subchunk2Size = " << h_tail.subchunk2Size <<
                ") not consistent with file size (ChunkSize + 8 = " << (h_head.ChunkSize + 8) << ")!\n";
        
        // correct the chunk sizes before attempting to read the samples... 
        h_head.ChunkSize = (uint32_t) fin_sz - 8;
        h_tail.subchunk2Size = h_head.ChunkSize - 36;

        if (logging.verbose)
            cout << "Recalculating the size of the data samples to " << h_head.ChunkSize << " bytes...\n";
    }

    // Warn if there is more than one channel with 16-bit samples and a sample frequency of 44.1 kHz
    if (!(h_head.numChannels == 1)) {

        cout << "*** WARNING***\n";
        cout << "The input file is a multi-channel " << h_head.bitsPerSample << "-bit PCM Wave file.\n";
        cout << "The last channel is assumed to contain the samples of interest. If this is not the case\n";
        cout << "then please modify the file to have only one channel and try again!\n\n";

    }

    // samples/channel: NumSamples * NumChannels * BitsPerSample / 8
    int samples_per_channel = h_tail.subchunk2Size / (h_head.numChannels * h_head.bitsPerSample / 8);
    int sample_byte_size = h_head.bitsPerSample / 8;
    int total_n_samples = h_tail.subchunk2Size / sample_byte_size;
    int n_sample_bytes = h_tail.subchunk2Size;
    if (logging.verbose) {
        cout << "Input file is a valid one channel " << h_head.bitsPerSample << "-bit PCM Wave file : \n";
        cout << "format: " << h_head.audioFormat << " (1 <=> PCM)\n";
        cout << "#channels: " << h_head.numChannels << " (1)\n";
        cout << "sample rate: " << h_head.sampleRate << " (44 100) \n";
        cout << "sample size: " << h_head.bitsPerSample << " (16)\n";
        cout << "#bytes: " << n_sample_bytes << "\n";
        cout << "#samples/channel: " << samples_per_channel << "\n";
    }

    sampleFreq = h_head.sampleRate;

    // Collect all samples into a vector 'samples'
    int n_samples = 0;
    samplesP = new Samples(samples_per_channel);
    if (h_head.numChannels == 1) {
        n_samples = samples_per_channel;
        if (sample_byte_size == 2) {
            // Read 16-bit samples
            Sample* samples_p = &(samplesP->front());
            fin.read((char*)samples_p, (streamsize) n_sample_bytes);           
        }
        else { // sample_byte_size == 1
            // Read 8-bit samples
            ByteSamples byte_samples(samples_per_channel);
            ByteSample* byte_samples_p = &byte_samples.front();
            fin.read((char*)byte_samples_p, (streamsize)n_sample_bytes);
            // Copy 8-bit samples into 16-bit sample vector
            for (int i = 0; i < samples_per_channel; i++)
                (*samplesP)[i] = (Sample) (((int)byte_samples[i] - 128) * 256); // scale from 8-bit unsigned to 16-bit signed sample}
        }
    }
    else { // several channels - assume the last one shall be used and skip all the other channels       
        if (sample_byte_size == 2) {
            Samples channel_samples(total_n_samples);
            Sample* channel_sample_p = &channel_samples.front();
            fin.read((char*)channel_sample_p, (streamsize)n_sample_bytes);
            SampleIter channel_sample_iter = channel_samples.begin();
            while (channel_sample_iter < channel_samples.end()) {
                // Skip samples for first channels
                if (channel_sample_iter < channel_samples.end() - (h_head.numChannels - 1))
                    channel_sample_iter += h_head.numChannels - 1;
                else
                    break;
                // Get sample for last channel
                (*samplesP)[n_samples++] = *channel_sample_iter++;
            }
        }
        else { // sample_byte_size == 1
            ByteSamples channel_samples(total_n_samples);
            ByteSample* channel_sample_p = &channel_samples.front();
            fin.read((char*)channel_sample_p, (streamsize) n_sample_bytes);
            ByteSampleIter channel_sample_iter = channel_samples.begin();
            while (channel_sample_iter < channel_samples.end()) {
                // Skip samples for first channels
                if (channel_sample_iter < channel_samples.end() - (h_head.numChannels - 1))
                    channel_sample_iter += h_head.numChannels - 1;
                else
                    break;
                // Get sample for last channel
                (*samplesP)[n_samples++] = (Sample) (((int) *channel_sample_iter++ - 128)*256); // scale to 16-bit sample value
            }
        }
    }

    fin.close();

    if (logging.verbose)
        cout << "Read " << n_samples << "...\n";

    return true;
}

bool PcmFile::writeSamples(string fileName, Samples *samples[], const int nChannels, int sampleFreq, Logging logging)
{
    // Check that each channel contains the same no of samples
    int n_samples = (int) samples[0]->size();

    for (int i = 1; i < nChannels; i++) {
        if (samples[i]->size() != n_samples) {
            cout << "Channel #" << i << " have different no of samples (" << samples[i]->size() << ") than channel #0 (" << n_samples << ")!\n";
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

    // Iterate over all samples, picking one sample per channel at a time,
    // and write it to PCM output file.
    if (nChannels == 1) {
        // Optimise for speed when there is only one channel to write
        Sample* samples_p = &samples[0]->front();
        fout.write((char*) samples_p, (streamsize)h_tail.subchunk2Size);
    }
    else {
        int sample_sz = sizeof(Sample);
        for (int s = 0; s < n_samples; s++) {
            for (int c = 0; c < nChannels; c++) {
                fout.write((char*)&(*samples[c])[s], sample_sz);
            }
        }
    }
    
    return true;

}

