
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "../shared/CommonTypes.h"
#include "ArgParser.h"
#include "../shared/WavEncoder.h"
#include "../shared/Debug.h"
#include "../shared/Utility.h"
#include "../shared/PcmFile.h"
#include "../shared/CSWCodec.h"

using namespace std;
using namespace std::filesystem;

#define HIGH_CSW_LEVEL 16384
#define LOW_CSW_LEVEL -16384

bool writePulse(Samples& samples, int& pos, Phase& phase, int pulseLen)
{
    uint16_t level = (phase == Phase::HighPhase ? HIGH_CSW_LEVEL : LOW_CSW_LEVEL);

    // Write pulse samples
    if (samples.size() < pos + pulseLen) {
        //cout << "RESIZE WHEN #SAMPLES ARE " << pos << ", SAMPLES SIZE " << samples.size() << " AND PULSE LENGTH IS " << pulseLen << "\n";
        samples.resize(pos + pulseLen);
    }
    for (int s = 0; s < pulseLen; s++) {
        samples[pos + s] = level;
    }

    // Next phase
    phase = (phase == Phase::HighPhase ? Phase::LowPhase : Phase::HighPhase);

    return true;

}


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

    if (arg_parser.verbose)
        cout << "Output file is: '" << arg_parser.dstFileName << "'\n";

    // Decode CSW file into pulse vector
    CSWCodec CSW_decoder = CSWCodec(arg_parser.verbose);
    Bytes pulses;
    int sample_freq;
    Phase phase;
    CSW_decoder.decode(arg_parser.srcFileName, pulses, sample_freq, phase);

    // Convert pulses into samples
    Samples samples(pulses.size() * 20); // Initially reserve 20 samples per 
    int pos = 0;
    int i = 0;
    while (i < pulses.size()) {
        if (pulses[i] != 0) {
            writePulse(samples, pos, phase, pulses[i]);
            pos += pulses[i];
            i++;
        }
        else if (i + 4 < pulses.size() && pulses[i] == 0) {
                int long_pulse = pulses[i + 1];
                long_pulse += pulses[i + 2] << 8;
                long_pulse += pulses[i + 3] << 16;
                long_pulse += pulses[i + 4] << 24;
                writePulse(samples, pos, phase, long_pulse);
                pos += long_pulse;
                i += 5;
            }
        else {
            cout << "Unexpected end of pulses!\n";
            return - 1;
        }
    }
    samples.resize(pos); // trim the samples to only include actual pulse samples

    // Write samples to WAV file
    Samples samples_v[] = { samples };
    if (!writeSamples(arg_parser.dstFileName, samples_v, 1, sample_freq, arg_parser.verbose)) {
        cout << "Failed to write samples to WAV file!\n";
        return -1;
    }

    return 0;
}





