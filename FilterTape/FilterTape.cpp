
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>
#include <chrono>
#include <cmath>


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
    
    if (arg_parser.logging.verbose) {
        cout << "Input file = '" << arg_parser.wavFile << "'\n";
        cout << "Output file = '" << arg_parser.outputFileName << "'\n";
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
    Samples* original_samples_p = NULL;
    int sample_freq = 44100;
    if (!PcmFile::readSamples(arg_parser.wavFile, original_samples_p, sample_freq, arg_parser.logging) || original_samples_p == NULL) {
        cout << "Couldn't open PCM Wave file '" << arg_parser.wavFile << "'\n";
        return -1;
    }
    if (arg_parser.logging.verbose)
        cout << "Samples read...\n";
    t_end = chrono::system_clock::now();
    dt = t_end - t_start;
    if (arg_parser.logging.verbose)
        cout << "Elapsed time: " << dt.count() << " seconds...\n";

    

    // Initialise sample filter
    Filter filter(sample_freq, arg_parser);

    Samples* samples_to_shape_p = original_samples_p;


    // Average the samples
    t_start = chrono::system_clock::now();
    Samples averaged_samples(original_samples_p->size());
    if (arg_parser.nAveragingSamples > 0) { 
        if (!filter.averageFilter(*original_samples_p, averaged_samples)) {
            cout << "Failed to filter samples!\n";
            if (original_samples_p != NULL)
                delete original_samples_p;
            return -1;
        }
        if (arg_parser.logging.verbose)
            cout << "Samples filtered...\n";
        samples_to_shape_p = &averaged_samples;
        t_end = chrono::system_clock::now();
        dt = t_end - t_start;
        if (arg_parser.logging.verbose)
            cout << "Elapsed time: " << dt.count() << " seconds...\n";
    }
    

    // Find extremums
    t_start = chrono::system_clock::now();
    ExtremumSamples extremums(original_samples_p->size());
    int n_extremums;
    if (!filter.normaliseFilter(*samples_to_shape_p, extremums, n_extremums)) {
        cout << "Failed to find extremums for samples!\n";
        if (original_samples_p != NULL)
            delete original_samples_p;
        return -1;
    }
    if (arg_parser.logging.verbose)
        cout << n_extremums << " (one every " << (int) round(original_samples_p->size() / n_extremums) << " samples)" << " extremums identified...\n";
    t_end = chrono::system_clock::now();
    dt = t_end - t_start;
    if (arg_parser.logging.verbose)
        cout << "Elapsed time: " << dt.count() << " seconds...\n";

    // Use found extremums to reconstruct the original samples
    t_start = chrono::system_clock::now();
    Samples shaped_samples(original_samples_p->size());
    if (!filter.plotFromExtremums(n_extremums, extremums, shaped_samples, (int)original_samples_p->size())) {
        cout << "Failed to plot from extremums!\n";
        if (original_samples_p != NULL)
            delete original_samples_p;
        return -1;
    }
    if (arg_parser.logging.verbose)
        cout << "Samples reshaped based on identified extremums...\n";
    t_end = chrono::system_clock::now();
    dt = t_end - t_start;
    if (arg_parser.logging.verbose)
        cout << "Elapsed time: " << dt.count() << " seconds...\n";

    // Write reconstructed samples to WAV file
    t_start = chrono::system_clock::now();
    bool success;
    if (arg_parser.outputMultipleChannels) {
        // Write original samples and the filtered samples into a multiple-channel 16-bit PCM output WAV file
        if (arg_parser.nAveragingSamples > 0) {
            Samples* samples_v[] = { original_samples_p , samples_to_shape_p, &shaped_samples};
            success = PcmFile::writeSamples(arg_parser.outputFileName, samples_v, (int) (end(samples_v) - begin(samples_v)), sample_freq, arg_parser.logging);
        }
        else {
            Samples *samples_v[] = { original_samples_p , &shaped_samples };
            success = PcmFile::writeSamples(arg_parser.outputFileName, samples_v, (int)(end(samples_v) - begin(samples_v)), sample_freq, arg_parser.logging);
        }
    }
    else {
        // Write the filtered samples into a one-channel 16-bit PCM output WAV file
        Samples *samples_v[] = { &shaped_samples };
        success = PcmFile::writeSamples(arg_parser.outputFileName, samples_v, (int)(end(samples_v) - begin(samples_v)), sample_freq, arg_parser.logging);
    }
    if (!success) {
        cout << "Couldn't write samples to Wave file '" << arg_parser.outputFileName << "'\n";
        if (original_samples_p != NULL)
            delete original_samples_p;
        return -1;
    }
    if (arg_parser.logging.verbose)
        cout << "Resulting samples written to file...\n";
    t_end = chrono::system_clock::now();
    dt = t_end - t_start;
    if (arg_parser.logging.verbose)
        cout << "Elapsed time: " << dt.count() << " seconds...\n";

    if (original_samples_p != NULL)
        delete original_samples_p;


    return 0;
}



