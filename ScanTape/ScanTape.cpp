
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
#include "BlockDecoder.h"
#include "FileDecoder.h"
#include "../shared/WaveSampleTypes.h"
#include "ArgParser.h"
#include "../shared/UEFCodec.h"
#include "../shared/AtomBasicCodec.h"
#include "../shared/DataCodec.h"
#include "../shared/PcmFile.h"
#include "../shared/Utility.h"
#include "../shared/CSWCodec.h"
#include "../shared/UEFCodec.h"
#include "WavTapeReader.h"
#include "UEFTapeReader.h"
#include "FileBlock.h"

using namespace std;
using namespace std::filesystem;






int main(int argc, const char* argv[])
{
    //const char* args[] = { "", "C:\\Users\\Joakim\\Documents\\BBC Micro\\Sources\\CONVOY.csw", "-g", "C:\\Users\\Joakim\\Documents\\BBC Micro\\Sources\\generated", "-bbm" };
    //ArgParser arg_parser = ArgParser(4, args);
    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;


    CycleDecoder* cycle_decoder_p = NULL;
    LevelDecoder* level_decoder_p = NULL;
    Samples samples;
    Bytes pulses;
    int sample_freq = 44100; // from CSW/WAV file but usually 44100 Hz;

    TapeReader* tape_reader = NULL;

    // Is it an UEF file?
    UEFCodec UEF_codec(arg_parser.verbose, arg_parser.targetMachine);
    bool UEF_file = false;
    Bytes UEF_data;
    if (UEF_codec.validUefFile(arg_parser.wavFile)) {
        if (arg_parser.verbose)
            cout << "UEF file detected - scanning it...\n";
        UEF_file = true;
        UEFTapeReader *UEF_tape_reader_p = new UEFTapeReader(UEF_codec, arg_parser);
        tape_reader = UEF_tape_reader_p;
    }
    // Is it a CSW file?
    else if (CSWCodec::isCSWFile(arg_parser.wavFile)) {
        if (arg_parser.verbose)
            cout << "CSW file detected - scanning it...\n";
        CSWCodec CSW_codec = CSWCodec(false, sample_freq, arg_parser.tapeTiming, arg_parser.verbose, arg_parser.targetMachine);
        Level first_half_cycle_level;
        if (!CSW_codec.decode(arg_parser.wavFile, pulses, first_half_cycle_level)) {
            cout << "Couldn't decode CSW Wave file '" << arg_parser.wavFile << "'\n";
            return -1;
        }

        cycle_decoder_p = new CSWCycleDecoder(sample_freq, first_half_cycle_level, pulses, arg_parser, arg_parser.verbose);
    }
    else // If not a CSW file it must be a WAV file
    {
        if (arg_parser.verbose)
            cout << "WAV file assumed - scanning it...\n";
        if (!PcmFile::readSamples(arg_parser.wavFile, samples, sample_freq, arg_parser.verbose)) {
            cout << "Couldn't open PCM Wave file '" << arg_parser.wavFile << "'\n";
            return -1;
        }

        // Create Level Decoder used to filter wave form into a well-defined level stream
        level_decoder_p = new LevelDecoder(sample_freq, samples, arg_parser.startTime, arg_parser);

        // Create Cycle Decoder used to produce a cycle stream from the level stream
        cycle_decoder_p = new WavCycleDecoder(sample_freq, *level_decoder_p, arg_parser);
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
        cout << "Tape timing to be used when generating UEF files = " << (arg_parser.tapeTiming.preserve ? "Original (from tape)" : "Standard") << "\n";
        cout << "Sample frequency from input file: " << sample_freq << " Hz\n";
        cout << "Target computer: " << _TARGET_MACHINE(arg_parser.targetMachine) << "\n";
    }

    TapeFile tape_file(ACORN_ATOM);

    // Create a reader based on CSW/WAV input as provided by a cycle decoder
    if (!UEF_file) {
        WavTapeReader *wav_tape_reader_p = new WavTapeReader(*cycle_decoder_p, 1200.0, arg_parser);
        tape_reader = wav_tape_reader_p;
    }


    // Create A Block Decoder used to detect and read one block from a tape reader
    BlockDecoder block_decoder(*tape_reader, arg_parser);

    // Create a File Decoder used to detect and read a complete Tape File
    FileDecoder fileDecoder(block_decoder, arg_parser, arg_parser.cat);

    // Create a log file
    ostream* fout_p = &cout;
    if (!arg_parser.cat) {

        string fout_name = "tape.log";
        filesystem::path fout_d = arg_parser.genDir;
        fout_d /= fout_name;
        string fout_path = fout_d.string();

        fout_p = new ofstream(fout_path);
        if (!*fout_p) {
            cout << "can't write to log file " << fout_name << "\n";
            return false;
        }
    }

    if (!arg_parser.cat) {
        *fout_p << "Input file = '" << arg_parser.wavFile << "'\n";
        *fout_p << "Start time = " << arg_parser.startTime << "\n";
        *fout_p << "Baudrate = " << arg_parser.tapeTiming.baudRate << "\n";
        *fout_p << "Frequency tolerance = " << arg_parser.freqThreshold << "\n";
        *fout_p << "Schmitt-trigger level tolerance = " << arg_parser.levelThreshold << "\n";
        *fout_p << "Min lead tone duration of first block= " << arg_parser.tapeTiming.minBlockTiming.firstBlockLeadToneDuration << " s\n";
        *fout_p << "Min lead tone duration of subsequent blocks = " << arg_parser.tapeTiming.minBlockTiming.otherBlockLeadToneDuration << " s\n";
        *fout_p << "Tape timing when generating UEF files = " << (arg_parser.tapeTiming.preserve ? "Original" : "Standard") << "\n";
    }

    // Read complete tape files using the File Decoder
    bool read_file;
    bool selected_file_found = false;
    while ((read_file = fileDecoder.readFile(*fout_p, tape_file, arg_parser.find_file_name))) {

        if (
            !arg_parser.cat &&
            tape_file.blocks.size() > 0 &&
            (arg_parser.find_file_name == "" || tape_file.blocks[0].blockName() == arg_parser.find_file_name)
            ) {

            selected_file_found = true;
            if (!read_file) {
                cout << "Failed to scan the WAV file!\n";
                //return -1;
            }
            if (arg_parser.verbose)
                cout << (tape_file.fileType == ACORN_ATOM ? "Atom" : "BBC Micro") << " Tape File '" << tape_file.blocks.front().blockName() <<
                "' read. Base file name used for generated files is: '" << tape_file.validFileName << "'.\n";

            DataCodec DATA_codec = DataCodec(arg_parser.verbose);
            string DATA_file_name = Utility::crEncodedFileNamefromDir(arg_parser.genDir, tape_file, "dat");
            if (!DATA_codec.encode(tape_file, DATA_file_name)) {
                cout << "Failed to write the DATA file!\n";
                //return -1;
            }

            AtomBasicCodec ABC_codec = AtomBasicCodec(arg_parser.verbose, arg_parser.targetMachine);
            string ABC_file_name;
            if (tape_file.fileType == ACORN_ATOM)
                ABC_file_name = Utility::crEncodedFileNamefromDir(arg_parser.genDir, tape_file, "abc");
            else if (tape_file.fileType <= BBC_MASTER)
                ABC_file_name = Utility::crEncodedFileNamefromDir(arg_parser.genDir, tape_file, "bbc");
            else {
                cout << "Unknwon target machine " << hex << tape_file.fileType << "\n";
                return -1;
            }
            if (!ABC_codec.encode(tape_file, ABC_file_name)) {
                cout << "Failed to write the program file!\n";
                //return -1;
            }

            if (tape_file.complete) { // Only generate files if the Tape file was completed (without missing blocks)

                // Create TAP file (Acorn Atom only)
                if (arg_parser.targetMachine == ACORN_ATOM) {
                    TAPCodec TAP_codec = TAPCodec(arg_parser.verbose);
                    string TAP_file_name = Utility::crEncodedFileNamefromDir(arg_parser.genDir, tape_file, "tap");
                    if (!TAP_codec.encode(tape_file, TAP_file_name)) {
                        cout << "Failed to write the TAP file!\n";
                        //return -1;
                    }
                }

                // Create UEF file
                UEFCodec UEF_codec = UEFCodec(arg_parser.tapeTiming.preserve, arg_parser.verbose, arg_parser.targetMachine);
                string UEF_file_name = Utility::crEncodedFileNamefromDir(arg_parser.genDir, tape_file, "uef");
                if (!UEF_codec.encode(tape_file, UEF_file_name)) {
                    cout << "Failed to write the UEF file!\n";
                    //return -1;
                }

                // Create binary file
                string BIN_file_name = Utility::crEncodedFileNamefromDir(arg_parser.genDir, tape_file, "");
                if (!TAPCodec::data2Binary(tape_file, BIN_file_name)) {
                    cout << "can't create Binary file " << BIN_file_name << "\n";
                    //return -1;
                }


            }


        }
        else if (arg_parser.cat && tape_file.blocks.size() > 0) {
            tape_file.logTAPFileHdr();
        }

    }

    if (arg_parser.find_file_name != "" && !selected_file_found)
        cout << "Couldn't find tape file '" << arg_parser.find_file_name << "!'\n";

    
    if (tape_reader != NULL)
        delete tape_reader;

    if (cycle_decoder_p != NULL)
        delete cycle_decoder_p;

    if (level_decoder_p != NULL)
        delete level_decoder_p;

    return 0;

    if (fout_p != &cout) {
        ((ofstream *) fout_p) -> close();
        delete fout_p;
    }

    return 0;
}



