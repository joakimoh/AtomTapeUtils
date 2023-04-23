
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "LevelDecoder.h"
#include "CycleDecoder.h"
#include "BlockDecoder.h"
#include "FileDecoder.h"
#include "../shared/WaveSampleTypes.h"
#include "ArgParser.h"
#include "../shared/UEFCodec.h"
#include "../shared/AtomBasicCodec.h"
#include "../shared/DataCodec.h"
#include "../shared/PcmFile.h"
#include "../shared/Utility.h"
#include "../shared/MMCCodec.h"

using namespace std;
using namespace std::filesystem;






int main(int argc, const char* argv[])
{
   
    
    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    cout << "Start time = " << arg_parser.mStartTime << "\n";
    cout << "Input file = '" << arg_parser.mWavFile << "'\n";
    cout << "Generate directory path = " << arg_parser.mGenDir << "\n";
    cout << "Baudrate = " << arg_parser.tapeTiming.baudRate << "\n";
    cout << "Debug time range = [" << encodeTime(arg_parser.mDbgStart) << ", " <<  encodeTime(arg_parser.mDbgEnd) << "]\n";
    cout << "Frequency tolerance = " << arg_parser.mFreqThreshold << "\n";
    cout << "Schmitt-trigger level tolerance = " << arg_parser.mLevelThreshold << "\n";
    cout << "Min lead tone duration of first block = " << arg_parser.tapeTiming.minBlockTiming.firstBlockLeadToneDuration << " s\n";
    cout << "Min lead tone duration of subsequent blocks = " << arg_parser.tapeTiming.minBlockTiming.otherBlockLeadToneDuration << " s\n";
    cout << "Min trailer tone duration = " << arg_parser.tapeTiming.minBlockTiming.trailerToneDuration << " s\n";
    cout << "Min micro lead duration = " << arg_parser.tapeTiming.minBlockTiming.microLeadToneDuration << " s\n";
 

    //
    // Open input file if it is a valid 16-bit PCM WAV file
    //

    Samples samples;
    if (!readSamples(arg_parser.mWavFile, samples)) {
        cout << "Couldn't open PCM Wave file '" << arg_parser.mWavFile << "'\n";
        return -1;
    }

    // Create Level Decoder used to filter wave form into a well-defined level stream
    LevelDecoder level_decoder(samples, arg_parser.mStartTime, arg_parser);

    // Create Cycle Decoder used to produce a cycle stream from the level stream
    CycleDecoder cycle_decoder(level_decoder, arg_parser);

    // Create A Block Decoder used to detect and read one block from a cycle stream
    BlockDecoder block_decoder(cycle_decoder, arg_parser);

    // Create a File Decoder used to detect and read a complete Atom Tape File
    FileDecoder fileDecoder(block_decoder, arg_parser);


    // Create a log file
    string fout_name = "tape.log";
    filesystem::path fout_p = arg_parser.mGenDir;
    fout_p /= fout_name;
    string fout_path = fout_p.string();

    ofstream fout(fout_path);
    if (!fout) {
        cout << "can't write to log file " << fout_name << "\n";
        return false;
    }

    fout << "Input file = '" << arg_parser.mWavFile << "'\n";
    fout << "Start time = " << arg_parser.mStartTime << "\n";
    fout << "Baudrate = " << arg_parser.tapeTiming.baudRate << "\n";
    fout << "Frequency tolerance = " << arg_parser.mFreqThreshold << "\n";
    fout << "Schmitt-trigger level tolerance = " << arg_parser.mLevelThreshold << "\n";
    fout << "Min lead tone duration of first block= " << arg_parser.tapeTiming.minBlockTiming.firstBlockLeadToneDuration << " s\n";
    fout << "Min lead tone duration of subsequent blocks = " << arg_parser.tapeTiming.minBlockTiming.otherBlockLeadToneDuration << " s\n";
    fout << "Min trailer tone duration = " << arg_parser.tapeTiming.minBlockTiming.trailerToneDuration << " s\n";
    fout << "Min micro lead duration = " << arg_parser.tapeTiming.minBlockTiming.microLeadToneDuration << " s\n";


    // Read complete Atom files using the File Decoder
    while (!level_decoder.endOfSamples()) {

        
        bool read_file = fileDecoder.readFile(fout);
        TAPFile tapFile;
        if (!fileDecoder.getTAPFile(tapFile)) {
            cout << "Failed to get TAP file\n";
            return -1;
        }


            if (tapFile.blocks.size() > 0) {

                if (!read_file) {
                    cout << "Failed to scan the WAV file!\n";
                    //return -1;
                }
                if (arg_parser.tracing)
                    cout << "Atom Tape File '" << tapFile.blocks.front().hdr.name <<
                        "' read. Base file name used for generated files is: '" << tapFile.validFileName << "'.\n";

                DataCodec DATA_codec = DataCodec(tapFile);
                string DATA_file_name = crEncodedFileNamefromDir(arg_parser.mGenDir, tapFile, "dat");
                if (!DATA_codec.encode(DATA_file_name)) {
                    cout << "Failed to write the DATA file!\n";
                    //return -1;
                }

                AtomBasicCodec ABC_codec = AtomBasicCodec(tapFile);
                string ABC_file_name = crEncodedFileNamefromDir(arg_parser.mGenDir, tapFile, "abc");
                if (!ABC_codec.encode(ABC_file_name)) {
                    cout << "Failed to write the program file!\n";
                    //return -1;
                }

                if (tapFile.complete) {

                    // Only generate TAP & UEF files if the Tape file was completed (without missing blocks)

                    TAPCodec TAP_codec = TAPCodec(tapFile);
                    string TAP_file_name = crEncodedFileNamefromDir(arg_parser.mGenDir, tapFile, "tap");
                    if (!TAP_codec.encode(TAP_file_name)) {
                        cout << "Failed to write the TAP file!\n";
                        //return -1;
                    }

                    UEFCodec UEF_codec = UEFCodec(tapFile);
                    string UEF_file_name = crEncodedFileNamefromDir(arg_parser.mGenDir, tapFile, "uef");
                    if (!UEF_codec.encode(UEF_file_name)) {
                        cout << "Failed to write the UEF file!\n";
                        //return -1;
                    }

                    MMCCodec MMC_codec(tapFile);
                    string MMC_file_name = crEncodedFileNamefromDir(arg_parser.mGenDir, tapFile, "");

                    if (!MMC_codec.encode(MMC_file_name)) {
                        cout << "Failed to write the MMC file!\n";
                        // return -1;
                    }
                }

                

                

            }

    

    }

    fout.close();

    return 0;
}



