
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "../shared/LevelDecoder.h"
#include "../shared/WavCycleDecoder.h"
#include "../shared/CSWCycleDecoder.h"
#include "../shared/CommonTypes.h"
#include "ArgParser.h"
#include "../shared/WavEncoder.h"
#include "../shared/CSWCodec.h"
#include "../shared/Logging.h"
#include "../shared/Utility.h"
#include "../shared/PcmFile.h"

using namespace std;
using namespace std::filesystem;



/*
 *
 * Create CSW file from A WAV file
 * *
 */
int main(int argc, const char* argv[])
{


    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    TapeProperties dummy_tape_properties;
    CSWCodec CSW_codec(arg_parser.mSampleFreq, dummy_tape_properties, arg_parser.logging, TargetMachine::UNKNOWN_TARGET);

    // Read samples
    Samples *samples_p = NULL;
    if (!PcmFile::readSamples(arg_parser.srcFileName, samples_p, arg_parser.mSampleFreq, arg_parser.logging) || samples_p == NULL) {
        cout << "Couldn't open PCM Wave file '" << arg_parser.srcFileName << "'\n";
        return -1;
    }

    // Create Level Decoder used to filter wave form into a well-defined level stream
    LevelDecoder level_decoder(arg_parser.mSampleFreq, *samples_p, 0.0, 0.1, 0.0, arg_parser.logging);
 
    // Create Cycle Decoder used to produce a cycle stream from the level stream
    WavCycleDecoder cycle_decoder(arg_parser.mSampleFreq, level_decoder, 0.1, arg_parser.logging);
    
    // Write samples to buffer
    while (cycle_decoder.advanceHalfCycle()) {
        HalfCycleInfo half_cycle_info = cycle_decoder.getHalfCycle();
        CSW_codec.writeHalfCycle(half_cycle_info.duration);
    }

    // Write samples to file
    if (!CSW_codec.writeSamples(arg_parser.dstFileName)) {
        cout << "Failed to write samples to file '" << arg_parser.dstFileName << "'!\n";
        if (samples_p != NULL)
            delete samples_p;
        return -1;
    }

    if (samples_p != NULL)
        delete samples_p;

    return 0;
}



