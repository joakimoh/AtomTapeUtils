
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
#include "../shared/BlockDecoder.h"
#include "../shared/FileDecoder.h"
#include "../shared/WaveSampleTypes.h"
#include "ArgParser.h"
#include "../shared/UEFCodec.h"
#include "../shared/AtomBasicCodec.h"
#include "../shared/DataCodec.h"
#include "../shared/PcmFile.h"
#include "../shared/Utility.h"
#include "../shared/CSWCodec.h"
#include "../shared/UEFCodec.h"
#include "../shared/WavTapeReader.h"
#include "../shared/UEFTapeReader.h"
#include "../shared/TAPCodec.h"
#include "../shared/FileBlock.h"
#include "../shared/WavEncoder.h"
#include "../shared/DiscCodec.h"
#include "../shared/BinCodec.h"

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
    Samples *samples_p = NULL;
    Bytes pulses;
    int sample_freq = 44100; // from CSW/WAV file but usually 44100 Hz;

    TapeReader* tape_reader = NULL;

    // Is it a UEF file?
    UEFCodec UEF_codec(arg_parser.logging, arg_parser.targetMachine);
    bool UEF_file = false;
    Bytes UEF_data;
    if (UEF_codec.validUefFile(arg_parser.wavFile)) {
        if (arg_parser.logging.verbose)
            cout << "UEF file detected - scanning it...\n";
        UEF_file = true;
        UEFTapeReader* UEF_tape_reader_p = new UEFTapeReader(
            UEF_codec, arg_parser.wavFile, arg_parser.logging, arg_parser.targetMachine
        );
        tape_reader = UEF_tape_reader_p;
    }
    // Is it a CSW file?
    else if (CSWCodec::isCSWFile(arg_parser.wavFile)) {
        if (arg_parser.logging.verbose)
            cout << "CSW file detected - scanning it...\n";
        CSWCodec CSW_codec = CSWCodec(false, sample_freq, arg_parser.tapeTiming, arg_parser.logging, arg_parser.targetMachine);
        Level first_half_cycle_level;
        if (!CSW_codec.decode(arg_parser.wavFile, pulses, first_half_cycle_level)) {
            cout << "Couldn't decode CSW Wave file '" << arg_parser.wavFile << "'\n";
            return -1;
        }

        cycle_decoder_p = new CSWCycleDecoder(
            sample_freq, first_half_cycle_level, pulses, arg_parser.freqThreshold, arg_parser.logging
        );
    }
    else // If not a CSW file it must be a WAV file
    {
        if (arg_parser.logging.verbose)
            cout << "WAV file assumed - scanning it...\n";
        if (!PcmFile::readSamples(arg_parser.wavFile, samples_p, sample_freq, arg_parser.logging)) {
            cout << "Couldn't open PCM Wave file '" << arg_parser.wavFile << "'\n";
            return -1;
        }

        // Create Level Decoder used to filter wave form into a well-defined level stream
        level_decoder_p = new LevelDecoder(
            sample_freq, *samples_p, arg_parser.startTime, arg_parser.freqThreshold, arg_parser.levelThreshold, arg_parser.logging
        );
 
        // Create Cycle Decoder used to produce a cycle stream from the level stream
        cycle_decoder_p = new WavCycleDecoder(
            sample_freq, *level_decoder_p, arg_parser.freqThreshold, arg_parser.logging
        );
    }

    if (arg_parser.logging.verbose) {
        cout << "Start time = " << arg_parser.startTime << "\n";
        cout << "Input file = '" << arg_parser.wavFile << "'\n";
        cout << "Generate directory path = " << arg_parser.genDir << "\n";
        cout << "Baudrate = " << arg_parser.tapeTiming.baudRate << "\n";
        cout << "Debug time range = [" << Utility::encodeTime(arg_parser.logging.start) << ", " << Utility::encodeTime(arg_parser.logging.end) << "]\n";
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
        WavTapeReader* wav_tape_reader_p = new WavTapeReader(*cycle_decoder_p, 1200.0, arg_parser.tapeTiming,
            arg_parser.targetMachine, arg_parser.logging
        );
        tape_reader = wav_tape_reader_p;
    }


    // Create A Block Decoder used to detect and read one block from a tape reader
    BlockDecoder block_decoder(*tape_reader, arg_parser.logging, arg_parser.targetMachine, arg_parser.limitBlockNo);

    // Create a File Decoder used to detect and read a complete Tape File
    FileDecoder fileDecoder(block_decoder, arg_parser.logging, arg_parser.targetMachine, arg_parser.tapeTiming, arg_parser.cat);

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

    // Prepare encoders for later use
    UEFCodec UEF_encoder(arg_parser.tapeTiming.preserve, arg_parser.logging, arg_parser.targetMachine);
    CSWCodec CSW_encoder(arg_parser.tapeTiming.preserve, 44100, arg_parser.tapeTiming, arg_parser.logging, arg_parser.targetMachine);
    WavEncoder WAV_encoder(arg_parser.tapeTiming.preserve, 44100, arg_parser.tapeTiming, arg_parser.logging, arg_parser.targetMachine);
    TAPCodec TAP_encoder(arg_parser.logging, arg_parser.targetMachine);

    bool genTapeFile = false;
    ostream* tfout_p = NULL;
    if (arg_parser.genUEF || arg_parser.genCSW || arg_parser.genWAV || arg_parser.genTAP) {
        genTapeFile = true;
        // Create the output file
        if (arg_parser.genUEF) {
            if (!UEF_encoder.openTapeFile(arg_parser.dstFileName)) {
                cout << "Failed to open UEF file '" << arg_parser.dstFileName << "' for writing!\n";
                return (-1);
            }
        }
        else if (arg_parser.genCSW) {
            if (!CSW_encoder.openTapeFile(arg_parser.dstFileName)) {
                cout << "Failed to open CSW file '" << arg_parser.dstFileName << "' for writing!\n";
                return (-1);
            }
        }
        else if (arg_parser.genWAV) {
            if (!WAV_encoder.openTapeFile(arg_parser.dstFileName)) {
                cout << "Failed to open WAV file '" << arg_parser.dstFileName << "' for writing!\n";
                return (-1);
            }
        }
        else if (arg_parser.genTAP) {
            if (!TAP_encoder.openTapeFile(arg_parser.dstFileName)) {
                cout << "Failed to open TAP file '" << arg_parser.dstFileName << "' for writing!\n";
                return (-1);
            }
        }
        else {
            // should never happen
            return (-1);
        }
    }

    // Read complete tape files using the File Decoder
    bool selected_file_found = false;
    vector<TapeFile> tape_files;
    vector<TapeFile> tape_files_complete;
    FileReadStatus read_status;
    while (fileDecoder.readFile(*fout_p, tape_file, arg_parser.searchedProgram, read_status)) {

        // If the file was read with some content then add it to the list of tape files
        if (tape_file.blocks.size() > 0 && (arg_parser.searchedProgram == "" || tape_file.header.name == arg_parser.searchedProgram)) {
            if (tape_file.complete) // save complete files
                tape_files_complete.push_back(tape_file);
            tape_files.push_back(tape_file); // also save incomplete files (to support recovery of damaged files)
        }
 
        selected_file_found = selected_file_found || (arg_parser.searchedProgram == "" || tape_file.header.name == arg_parser.searchedProgram);

    }

    if (arg_parser.searchedProgram != "" && !selected_file_found)
        cout << "Couldn't find tape file '" << arg_parser.searchedProgram << "'!\n";
    else if (!arg_parser.cat && arg_parser.searchedProgram != "")
        cout << "Found tape file '" << arg_parser.searchedProgram << "'; output file(s) will be generated!\n";

    // Should files be extracted only not generating disc image
    if (selected_file_found && !arg_parser.genSSD) {

        // Generate the different types of files (DATA, ABC/BBC, TAP, UEF, BIN) for the each file
        for (int i = 0; i < tape_files.size(); i++) {

            TapeFile& tape_file = tape_files[i];
      
            if (!genTapeFile && !arg_parser.cat) {

                // Creata DATA file
                DataCodec DATA_codec = DataCodec(arg_parser.logging);
                string DATA_file_name = Utility::crEncodedFileNamefromDir(arg_parser.genDir, tape_file, "dat");
                if (!DATA_codec.encode(tape_file, DATA_file_name)) {
                    cout << "Failed to write the DATA file!\n";
                    //return -1;
                }

                // Creata ABC/BBC program file
                AtomBasicCodec ABC_codec = AtomBasicCodec(arg_parser.logging, arg_parser.targetMachine);
                string ABC_file_name = Utility::crEncodedProgramFileNamefromDir(arg_parser.genDir, arg_parser.targetMachine, tape_file);
                if (!ABC_codec.detokenise(tape_file, ABC_file_name)) {
                    cout << "Failed to write the program file!\n";
                    //return -1;
                }

                // Create TAP file
                TAPCodec TAP_codec = TAPCodec(arg_parser.logging);
                string TAP_file_name = Utility::crEncodedFileNamefromDir(arg_parser.genDir, tape_file, "tap");
                if (!TAP_codec.encode(tape_file, TAP_file_name)) {
                    cout << "Failed to write the TAP file!\n";
                    //return -1;
                }

                // Create UEF file
                string UEF_file_name = Utility::crEncodedFileNamefromDir(arg_parser.genDir, tape_file, "uef");
                if (!UEF_encoder.encode(tape_file, UEF_file_name)) {
                    cout << "Failed to write the UEF file!\n";
                    //return -1;
                }

                // Create BIN file
                string BIN_file_name = Utility::crEncodedFileNamefromDir(arg_parser.genDir, tape_file, "bin");
                BinCodec BIN_codec(arg_parser.logging);
                if (!BIN_codec.encode(tape_file, BIN_file_name)) {
                    cout << "can't create Binary file " << BIN_file_name << "\n";
                    //return -1;
                }

            }

            else if (!genTapeFile && arg_parser.cat) {

                if (!tape_file.complete || tape_file.corrupted)
                    cout << "***";
                else
                    cout << "   ";
                // Log found file
                tape_file.logFileHdr();
            }

            else if (genTapeFile && tape_file.complete) {

                    // Add program to UEF/CSW/WAV tape file

                    if (arg_parser.genUEF) {
                        if (!UEF_encoder.encode(tape_file)) {
                            cout << "Failed to update the UEF file!\n";
                            UEF_encoder.closeTapeFile();
                            if (samples_p != NULL) delete samples_p;
                            return -1;
                        }
                    }
                    else if (arg_parser.genCSW) {
                        if (!CSW_encoder.encode(tape_file)) {
                            cout << "Failed to update the CSW file!\n";
                            CSW_encoder.closeTapeFile();
                            if (samples_p != NULL) delete samples_p;
                            return -1;
                        }
                    }
                    else if (arg_parser.genWAV) {
                        if (!WAV_encoder.encode(tape_file)) {
                            cout << "Failed to update the WAV file!\n";
                            WAV_encoder.closeTapeFile();
                            if (samples_p != NULL) delete samples_p;
                            return -1;
                        }
                    }
                    else if (arg_parser.genTAP) {
                        if (!TAP_encoder.encode(tape_file)) {
                            cout << "Failed to update the TAP file!\n";
                            WAV_encoder.closeTapeFile();
                            if (samples_p != NULL) delete samples_p;
                            return -1;
                        }
                    }
                    else {
                        if (samples_p != NULL) delete samples_p;
                        return (-1);
                    }
             }
            
        }
    }
    else if (selected_file_found) {
        // Create disc image from tape files
        DiscCodec DISC_codec = DiscCodec(arg_parser.logging);
        filesystem::path file_path = arg_parser.wavFile;
        string title = Utility::crReadableString(file_path.stem().string(), 12);
        if (!DISC_codec.write(title, arg_parser.dstFileName, tape_files_complete)) {
            cout << "Failed to create disc image!\n";
            if (samples_p != NULL) {
                delete samples_p;
            }
            return -1;
        }
    }

    
    if (tape_reader != NULL)
        delete tape_reader;

    if (cycle_decoder_p != NULL)
        delete cycle_decoder_p;

    if (level_decoder_p != NULL)
        delete level_decoder_p;


    // Close and delete log file - if applicable
    if (fout_p != &cout) {
        ((ofstream *) fout_p) -> close();
        delete fout_p;
    }
    if (tfout_p != NULL) {
        ((ofstream*)tfout_p)->close();
        delete tfout_p;
    }

    if (samples_p != NULL) {
        delete samples_p;
    }

    // Close output tape file (if applicable)
    if (arg_parser.genUEF) {
        if (!UEF_encoder.closeTapeFile()) {
            cout << "Failed to close UEF file '" << arg_parser.dstFileName << "'!\n";
            return (-1);
        }
    }
    else if (arg_parser.genCSW) {
        if (!CSW_encoder.closeTapeFile()) {
            cout << "Failed to close CSW file '" << arg_parser.dstFileName << "'!\n";
            return (-1);
        }
    }
    else if (arg_parser.genWAV) {
        if (!WAV_encoder.closeTapeFile()) {
            cout << "Failed to close WAV file '" << arg_parser.dstFileName << "'!\n";
            return (-1);
        }
    }
    else if (arg_parser.genTAP) {
        if (!TAP_encoder.closeTapeFile()) {
            cout << "Failed to close TAP file '" << arg_parser.dstFileName << "'!\n";
            return (-1);
        }
    }

    return 0;
}



