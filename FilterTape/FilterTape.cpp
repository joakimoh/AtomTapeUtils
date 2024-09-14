
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>
#include <chrono>


using namespace std;
using namespace std::filesystem;


#include "../shared/CommonTypes.h"
#include "../shared/PcmFile.h"
#include "ArgParser.h"
#include "Filter.h"



int main(int argc, const char* argv[])
{

    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return 0;
    
    if (arg_parser.verbose) {
        cout << "Input file = '" << arg_parser.wavFile << "'\n";
        cout << "Output file = '" << arg_parser.outputFileName << "'\n";
        cout << "Baud rate = " << arg_parser.baudRate << "\n";
        cout << "Derivative threshold = " << arg_parser.derivativeThreshold << "\n";
        if (arg_parser.nAveragingSamples > 0)
            cout << "No of averaging samples = " << arg_parser.nAveragingSamples * 2 + 1 << "\n";
        else
            cout << "No averaging of samples\n";
        cout << "Output multiple channels = " << (arg_parser.outputMultipleChannels == true ? "Yes" : "No") << "\n";
        cout << "High saturation level = " << round(100 * arg_parser.saturationLevelHigh) << " %\n";
        cout << "Low saturation level = " << round(100 * arg_parser.saturationLevelHigh) << " %\n";
        cout << "Min separation between peaks = " << round(100 * arg_parser.minPeakDistance) << " % of 2400 Hz cycle\n";
    }

    std::chrono::time_point<std::chrono::system_clock> t_start, t_end;
    chrono::duration<double> dt;

    // Open input file if it is a valid 16-bit PCM WAV file


    t_start = chrono::system_clock::now();
    Samples samples;
    int sample_freq = 44100;
    if (!PcmFile::readSamples(arg_parser.wavFile, samples, sample_freq, arg_parser.verbose)) {
        cout << "Couldn't open PCM Wave file '" << arg_parser.wavFile << "'\n";
        return -1;
    }
    if (arg_parser.verbose)
        cout << "Samples read...\n";
    t_end = chrono::system_clock::now();
    dt = t_end - t_start;
    if (arg_parser.verbose)
        cout << "Elapsed time: " << dt.count() << " seconds...\n";

    

    // Initialise sample filter
    Filter filter(sample_freq, arg_parser);

    Samples& samples_to_filter = samples;


    // Average the samples
    t_start = chrono::system_clock::now();
    Samples averaged_samples(samples.size());
    if (arg_parser.nAveragingSamples > 0) { 
        //averaged_samples.reserve(samples.size());
        if (!filter.averageFilter(samples, averaged_samples)) {
            cout << "Failed to filter samples!\n";
            return -1;
        }
        if (arg_parser.verbose)
            cout << "Samples filtered...\n";
        samples_to_filter = averaged_samples;
        t_end = chrono::system_clock::now();
        dt = t_end - t_start;
        if (arg_parser.verbose)
            cout << "Elapsed time: " << dt.count() << " seconds...\n";
    }
    

    // Find extremums
    t_start = chrono::system_clock::now();
    ExtremumSamples extremums(samples.size());
    int n_extremums;
    if (!filter.normaliseFilter(samples_to_filter, extremums, n_extremums)) {
        cout << "Failed to find extremums for samples!\n";
        return -1;
    }
    if (arg_parser.verbose)
        cout << n_extremums << " (one every " << (int) round(samples.size() / n_extremums) << " samples)" << " extremums identified...\n";
    t_end = chrono::system_clock::now();
    dt = t_end - t_start;
    if (arg_parser.verbose)
        cout << "Elapsed time: " << dt.count() << " seconds...\n";

    // Use found extremums to reconstruct the original samples
    t_start = chrono::system_clock::now();
    Samples new_shapes(samples.size());
    if (!filter.plotFromExtremums(n_extremums, extremums, new_shapes, (int) samples.size())) {
        cout << "Failed to plot from extremums!\n";
        return -1;
    }
    if (arg_parser.verbose)
        cout << "Samples reshaped based on identified extremums...\n";
    t_end = chrono::system_clock::now();
    dt = t_end - t_start;
    if (arg_parser.verbose)
        cout << "Elapsed time: " << dt.count() << " seconds...\n";

    // Write reconstructed samples to WAV file
    t_start = chrono::system_clock::now();
    bool success;
    if (arg_parser.outputMultipleChannels) {
        // Write original samples and the filtered samples into a multiple-channel 16-bit PCM output WAV file
        if (arg_parser.nAveragingSamples > 0) {
            Samples samples_v[] = { samples , samples_to_filter, new_shapes };
            success = PcmFile::writeSamples(arg_parser.outputFileName, samples_v, (int) (end(samples_v) - begin(samples_v)), sample_freq, arg_parser.verbose);
        }
        else {
            Samples samples_v[] = { samples , new_shapes };
            success = PcmFile::writeSamples(arg_parser.outputFileName, samples_v, (int)(end(samples_v) - begin(samples_v)), sample_freq, arg_parser.verbose);
        }
    }
    else {
        // Write the filtered samples into a one-channel 16-bit PCM output WAV file
        Samples samples_v[] = { new_shapes };
        success = PcmFile::writeSamples(arg_parser.outputFileName, samples_v, (int)(end(samples_v) - begin(samples_v)), sample_freq, arg_parser.verbose);
    }
    if (!success) {
        cout << "Couldn't write samples to Wave file '" << arg_parser.outputFileName << "'\n";
        return -1;
    }
    if (arg_parser.verbose)
        cout << "Resulting samples written to file...\n";
    t_end = chrono::system_clock::now();
    dt = t_end - t_start;
    if (arg_parser.verbose)
        cout << "Elapsed time: " << dt.count() << " seconds...\n";


    return 0;
}



