
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

#define HIGH_CSW_LEVEL 16384
#define LOW_CSW_LEVEL -16384

bool writePulse(Samples& samples, int& pos, Level& pulseLevel, int pulseLen)
{
    uint16_t level = (pulseLevel == Level::HighLevel ? HIGH_CSW_LEVEL : LOW_CSW_LEVEL);

    // Write pulse samples
    if (samples.size() < pos + pulseLen) {
        //cout << "RESIZE WHEN #SAMPLES ARE " << pos << ", SAMPLES SIZE " << samples.size() << " AND PULSE LENGTH IS " << pulseLen << "\n";
        samples.resize(pos + pulseLen);
    }
    for (int s = 0; s < pulseLen; s++) {
        samples[pos + s] = level;
    }

    // Next half_cycle
    pulseLevel = (pulseLevel == Level::HighLevel ? Level::LowLevel : Level::HighLevel);

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

    if (arg_parser.logging.verbose)
        cout << "Output file is: '" << arg_parser.dstFileName << "'\n";

   
    // Decode CSW file into pulse vector
    int sample_freq = 44100;
    CSWCodec CSW_decoder = CSWCodec(sample_freq, arg_parser.tapeTiming, arg_parser.logging, UNKNOWN_TARGET);
    Bytes pulses;
    
    Level half_cycle_level;
    CSW_decoder.decode(arg_parser.srcFileName, pulses, half_cycle_level);

    // Convert pulses into samples
    Samples samples(pulses.size() * 20); // Initially reserve 20 samples per 
    int pos = 0;
    int i = 0;
    while (i < pulses.size()) {
        if (pulses[i] != 0) {
            writePulse(samples, pos, half_cycle_level, pulses[i]);
            pos += pulses[i];
            i++;
        }
        else if (i + 4 < pulses.size() && pulses[i] == 0) {
                int long_pulse = pulses[i + 1];
                long_pulse += pulses[i + 2] << 8;
                long_pulse += pulses[i + 3] << 16;
                long_pulse += pulses[i + 4] << 24;
                writePulse(samples, pos, half_cycle_level, long_pulse);
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
    if (!PcmFile::writeSamples(arg_parser.dstFileName, samples_v, 1, sample_freq, arg_parser.logging)) {
        cout << "Failed to write samples to WAV file!\n";
        return -1;
    }

    return 0;
}





