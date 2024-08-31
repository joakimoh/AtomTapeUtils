
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "LevelDecoder.h"
#include "WavCycleDecoder.h"
#include "CSWCycleDecoder.h"
#include "AtomBlockDecoder.h"
#include "BBMBlockDecoder.h"
#include "AtomFileDecoder.h"
#include "BBMFileDecoder.h"
#include "../shared/WaveSampleTypes.h"
#include "ArgParser.h"
#include "../shared/UEFCodec.h"
#include "../shared/AtomBasicCodec.h"
#include "../shared/DataCodec.h"
#include "../shared/PcmFile.h"
#include "../shared/Utility.h"
#include "../shared/CSWCodec.h"
#include "BlockTypes.h"

using namespace std;
using namespace std::filesystem;






int main(int argc, const char* argv[])
{
   
    
    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

 
    CycleDecoder* cycle_decoder = NULL;
    LevelDecoder* level_decoder = NULL;
    Samples samples;
    Bytes pulses;
    int sample_freq; // from CSW/WAV file but usually 44100 Hz;

    // Is it a CSW file?
    if (CSWCodec::isCSWFile(arg_parser.wavFile)) {
        if (arg_parser.verbose)
            cout << "CSW file detected - scanning it...\n";
        CSWCodec CSW_codec = CSWCodec(arg_parser.verbose, arg_parser.targetMachine);
        HalfCycle first_half_cycle;
        if (!CSW_codec.decode(arg_parser.wavFile, pulses, sample_freq, first_half_cycle)) {
            cout << "Couldn't decode CSW Wave file '" << arg_parser.wavFile << "'\n";
            return -1;
        }

        cycle_decoder = new CSWCycleDecoder(sample_freq, first_half_cycle, pulses, arg_parser, arg_parser.verbose);
    }
    else // If not a CSW file it must be a WAV file
    {
        if (arg_parser.verbose)
            cout << "WAV file assumed - scanning it...\n";
        if (!readSamples(arg_parser.wavFile, samples, sample_freq, arg_parser.verbose)) {
            cout << "Couldn't open PCM Wave file '" << arg_parser.wavFile << "'\n";
            return -1;
        }

        // Create Level Decoder used to filter wave form into a well-defined level stream
        level_decoder = new LevelDecoder(sample_freq, samples, arg_parser.startTime, arg_parser);

        // Create Cycle Decoder used to produce a cycle stream from the level stream
        cycle_decoder = new WavCycleDecoder(sample_freq , *level_decoder, arg_parser);
    }
    if (arg_parser.verbose) {
        cout << "Start time = " << arg_parser.startTime << "\n";
        cout << "Input file = '" << arg_parser.wavFile << "'\n";
        cout << "Generate directory path = " << arg_parser.genDir << "\n";
        cout << "Baudrate = " << arg_parser.tapeTiming.baudRate << "\n";
        cout << "Debug time range = [" << Utility::encodeTime(arg_parser.dbgStart) << ", " << Utility::encodeTime(arg_parser.dbgEnd) << "]\n";
        cout << "Frequency tolerance = " << arg_parser.freqThreshold << "\n";
        cout << "Schmitt-trigger level tolerance = " << arg_parser.levelThreshold << "\n";
        cout << "Min lead tone duration of first block = " << arg_parser.tapeTiming.minBlockTiming.firstBlockLeadToneDuration << " s\n";
        cout << "Min lead tone duration of subsequent blocks = " << arg_parser.tapeTiming.minBlockTiming.otherBlockLeadToneDuration << " s\n";
        cout << "Min micro lead duration = " << arg_parser.tapeTiming.minBlockTiming.microLeadToneDuration << " s\n";
        cout << "Tape timing when generating UEF/CSW files = " << (arg_parser.tapeTiming.preserve ? "Original" : "Standard") << "\n";
        cout << "Sample frequency from input file: " << sample_freq << " Hz\n";
        cout << "Target computer: " << (arg_parser.targetMachine?"BBC Micro":"Acorn Atom") << "\n";
    }
   
    FileDecoder *fileDecoder = NULL;
    TapeFile *tapFile = NULL;

    if (arg_parser.targetMachine <= BBC_MASTER) {

        // Create A Block Decoder used to detect and read one block from a cycle stream
        BBMBlockDecoder block_decoder(*cycle_decoder, arg_parser, arg_parser.verbose);

        // Create a File Decoder used to detect and read a complete Atom Tape File
        fileDecoder = new BBMFileDecoder(block_decoder, arg_parser);

        tapFile = new TapeFile(BBC_MODEL_B);
    }
    else if (arg_parser.targetMachine == ACORN_ATOM) {

        // Create A Block Decoder used to detect and read one block from a cycle stream
        AtomBlockDecoder block_decoder(*cycle_decoder, arg_parser, arg_parser.verbose);

        // Create a File Decoder used to detect and read a complete Atom Tape File
        fileDecoder = new AtomFileDecoder(block_decoder, arg_parser);

        tapFile = new TapeFile(ACORN_ATOM);
    }
    else {
        cout << "Unknown target machine " << hex << arg_parser.targetMachine << "!\n";
        return -1;
    }
 

    // Create a log file
    string fout_name = "tape.log";
    filesystem::path fout_p = arg_parser.genDir;
    fout_p /= fout_name;
    string fout_path = fout_p.string();

    ofstream fout(fout_path);
    if (!fout) {
        cout << "can't write to log file " << fout_name << "\n";
        return false;
    }

    fout << "Input file = '" << arg_parser.wavFile << "'\n";
    fout << "Start time = " << arg_parser.startTime << "\n";
    fout << "Baudrate = " << arg_parser.tapeTiming.baudRate << "\n";
    fout << "Frequency tolerance = " << arg_parser.freqThreshold << "\n";
    fout << "Schmitt-trigger level tolerance = " << arg_parser.levelThreshold << "\n";
    fout << "Min lead tone duration of first block= " << arg_parser.tapeTiming.minBlockTiming.firstBlockLeadToneDuration << " s\n";
    fout << "Min lead tone duration of subsequent blocks = " << arg_parser.tapeTiming.minBlockTiming.otherBlockLeadToneDuration << " s\n";
    fout << "Tape timing when generated UEF/CSW files = " << (arg_parser.tapeTiming.preserve?"Original":"Standard") << "\n";
    
    // Read complete Atom files using the File Decoder
    bool read_file;
    while ((read_file = fileDecoder->readFile(fout, *tapFile))) {


            if (tapFile->blocks.size() > 0) {

                if (!read_file) {
                    cout << "Failed to scan the WAV file!\n";
                    //return -1;
                }
                if (arg_parser.verbose)
                    cout << (tapFile->fileType == ACORN_ATOM ?"Atom":"BBC Micro") << " Tape File '" << tapFile->blocks.front().blockName() <<
                        "' read. Base file name used for generated files is: '" << tapFile->validFileName << "'.\n";

                DataCodec DATA_codec = DataCodec(arg_parser.verbose);
                string DATA_file_name = Utility::crEncodedFileNamefromDir(arg_parser.genDir, *tapFile, "dat");
                if (!DATA_codec.encode(*tapFile, DATA_file_name)) {
                    cout << "Failed to write the DATA file!\n";
                    //return -1;
                }

                AtomBasicCodec ABC_codec = AtomBasicCodec(arg_parser.verbose, arg_parser.targetMachine);
                string ABC_file_name;
                if (tapFile->fileType == ACORN_ATOM)
                    ABC_file_name = Utility::crEncodedFileNamefromDir(arg_parser.genDir, *tapFile, "abc");
                else if (tapFile->fileType <= BBC_MASTER)
                    ABC_file_name = Utility::crEncodedFileNamefromDir(arg_parser.genDir, *tapFile, "bbc");
                else {
                    cout << "Unknwon target machine " << hex << tapFile->fileType << "\n";
                    return -1;
                }
                if (!ABC_codec.encode(*tapFile, ABC_file_name)) {
                    cout << "Failed to write the program file!\n";
                    //return -1;
                }

                if (tapFile->complete) { // Only generate files if the Tape file was completed (without missing blocks)

                    // Create TAP file (Acorn Atom only)
                    if (arg_parser.targetMachine == ACORN_ATOM) {
                        TAPCodec TAP_codec = TAPCodec(arg_parser.verbose);
                        string TAP_file_name = Utility::crEncodedFileNamefromDir(arg_parser.genDir, *tapFile, "tap");
                        if (!TAP_codec.encode(*tapFile, TAP_file_name)) {
                            cout << "Failed to write the TAP file!\n";
                            //return -1;
                        }
                    }

                    // Create UEF file
                    UEFCodec UEF_codec = UEFCodec(arg_parser.tapeTiming.preserve, arg_parser.verbose, arg_parser.targetMachine);
                    string UEF_file_name = Utility::crEncodedFileNamefromDir(arg_parser.genDir, *tapFile, "uef");
                    if (!UEF_codec.encode(*tapFile, UEF_file_name)) {
                        cout << "Failed to write the UEF file!\n";
                        //return -1;
                    }

                    // Create binary file
                    string BIN_file_name = Utility::crEncodedFileNamefromDir(arg_parser.genDir, *tapFile, "");
                    if (!TAPCodec::data2Binary(*tapFile, BIN_file_name)) {
                        cout << "can't create Binary file " << BIN_file_name << "\n";
                        //return -1;
                    }


                }

                

                

            }

    

    }

    fout.close();

    return 0;
}



