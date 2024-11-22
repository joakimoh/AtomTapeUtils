
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>
#include <cstdint>

#include <math.h>

#include "../shared/CommonTypes.h"
#include "ArgParser.h"
#include "../shared/WavEncoder.h"
#include "../shared/Logging.h"
#include "../shared/Utility.h"
#include "../shared/PcmFile.h"
#include "../shared/CSWCodec.h"

using namespace std;
using namespace std::filesystem;

/*
 *
 * Create WAV file from CSW file
 * *
 */
int main(int argc, const char* argv[])
{


    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    if (arg_parser.logging.verbose) {
        if (arg_parser.outputPulses)
            cout << "Pulses will be generated...\n";
        else
            cout << "1/2 cycles will be generated...\n";
        cout << "Output file is: '" << arg_parser.dstFileName << "'\n";
    }

   
    // Decode CSW file into pulse vector
    int sample_freq = 44100;
    CSWCodec CSW_decoder = CSWCodec(sample_freq, arg_parser.tapeTiming, arg_parser.logging, UNKNOWN_TARGET);
    Bytes pulses;
    
    Level half_cycle_level;
    CSW_decoder.decode(arg_parser.srcFileName, pulses, half_cycle_level);

    // Convert pulses into samples
    Samples samples;// (pulses.size() * 20); // Initially reserve 20 samples per pulse
    int pos = 0;
    int i = 0;
    int pulse_count = 0;
    while (i < pulses.size()) {
        double t = (double) pos / sample_freq;
        int n_samples;
        Level l = half_cycle_level;
        if (pulses[i] != 0) {
            n_samples = pulses[i];        
            pos += pulses[i];
            i++;
        }
        else if (i + 4 < pulses.size() && pulses[i] == 0) {
                n_samples = pulses[i + 1];
                n_samples += pulses[i + 2] << 8;
                n_samples += pulses[i + 3] << 16;
                n_samples += pulses[i + 4] << 24;
                pos += n_samples;
                i += 5;
                if (arg_parser.logging.verbose)
                    cout << "Long " << _LEVEL(l) << " Pulse #" << pulse_count << ": " << n_samples << " at byte " << i-5 << " (" << Utility::encodeTime(t) << ")\n";
            }
        else {
            cout << "Unexpected end of pulses!\n";
            return - 1;
        }
        if (arg_parser.outputPulses) {
            if (!WavEncoder::writePulse(samples, half_cycle_level, n_samples)) {
                cout << "Failed to write " << n_samples << " " << _LEVEL(half_cycle_level) << " samples for a pulse\n";
                return -1;
            }
        }
        else {
            if (!WavEncoder::writeHalfCycle(samples, half_cycle_level, n_samples)) {
                cout << "Failed to write " << n_samples << " " << _LEVEL(half_cycle_level) << " samples for a 1/2 cycle\n";
                return -1;
            }
        }
        pulse_count++;
    }
    samples.resize(pos); // trim the samples to only include actual pulse samples

    if (arg_parser.logging.verbose) {
        cout << "A total of " << pulse_count << " pulses read\n";
    }

    // Write samples to WAV file
    Samples *samples_v[] = { &samples };
    if (!PcmFile::writeSamples(arg_parser.dstFileName, samples_v, 1, sample_freq, arg_parser.logging)) {
        cout << "Failed to write samples to WAV file!\n";
        return -1;
    }

    return 0;
}





