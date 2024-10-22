
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "../shared/CommonTypes.h"
#include "ArgParser.h"
#include "../shared/TAPCodec.h"
#include "../shared/Logging.h"
#include "../shared/UEFCodec.h"
#include "../shared/AtomBasicCodec.h"
#include "../shared/DataCodec.h"
#include "../shared/PcmFile.h"
#include "../shared/Utility.h"
#include "../shared/MMCCodec.h"
#include "../shared/CSWCodec.h"
#include "../shared/WavEncoder.h"
#include "../shared/DiscCodec.h"

using namespace std;
using namespace std::filesystem;



/*
 *
 * Create DATA file from TAP file
 * *
 */
int main(int argc, const char* argv[])
{


    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    if (arg_parser.logging.verbose)
        cout << "Output dir = " << arg_parser.dstDir << "\n";

    // Prepare encoders for later use
    UEFCodec UEF_encoder(false, arg_parser.logging, arg_parser.targetMachine);
    CSWCodec CSW_encoder(false, 44100, arg_parser.tapeTiming, arg_parser.logging, arg_parser.targetMachine);
    WavEncoder WAV_encoder(false, 44100, arg_parser.tapeTiming, arg_parser.logging, arg_parser.targetMachine);

    bool genTapeFile = false;
    ostream* tfout_p = NULL;
    if (arg_parser.genUEF || arg_parser.genCSW || arg_parser.genWAV) {
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
        else {
            // should never happen
            return (-1);
        }
    }

    TAPCodec TAP_codec = TAPCodec(arg_parser.logging);

    // Scan TAP file for Atom files
    vector<TapeFile> tape_files;
    TAP_codec.decodeMultipleFiles(arg_parser.srcFileName, tape_files);
    if (arg_parser.logging.verbose)
        cout << "#TAP files = " << tape_files.size() << "\n";

    // Iterate over the collected files
    bool selected_file_found = false;
    if (!arg_parser.genSSD) {
        for (int i = 0; i < tape_files.size(); i++) {

            TapeFile& tape_file = tape_files[i];

            selected_file_found = selected_file_found || (arg_parser.searchedProgram == "" || tape_file.programName == arg_parser.searchedProgram);


            if ((arg_parser.searchedProgram == "" || tape_file.programName == arg_parser.searchedProgram)) {

                string host_file_name = TapeFile::crValidHostFileName(tape_file.programName);

                if (!genTapeFile && !arg_parser.cat) {

                    // Generate the different types of files (DATA, ABC/BBC, TAP, UEF, BIN) for the found file

                    if (arg_parser.logging.verbose)
                        cout << "Atom Tape File '" << tape_file.blocks.front().atomHdr.name <<
                        "' read. Base file name used for generated files is: '" << host_file_name << "'.\n";

                    // Creata DATA file
                    DataCodec DATA_codec = DataCodec(arg_parser.logging);
                    string DATA_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tape_file, "dat");
                    if (!DATA_codec.encode(tape_file, DATA_file_name)) {
                        cout << "Failed to write the DATA file!\n";
                        //return -1;
                    }

                    // Creata ABC/BBC program file
                    AtomBasicCodec ABC_codec = AtomBasicCodec(arg_parser.logging, arg_parser.targetMachine);
                    string ABC_file_name = Utility::crEncodedProgramFileNamefromDir(arg_parser.dstDir, arg_parser.targetMachine, tape_file);
                    if (!ABC_codec.encode(tape_file, ABC_file_name)) {
                        cout << "Failed to write the program file!\n";
                        //return -1;
                    }

                    // Create TAP file (Acorn Atom only)
                    if (arg_parser.targetMachine == ACORN_ATOM) {
                        TAPCodec TAP_codec = TAPCodec(arg_parser.logging);
                        string TAP_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tape_file, "tap");
                        if (!TAP_codec.encode(tape_file, TAP_file_name)) {
                            cout << "Failed to write the TAP file!\n";
                            //return -1;
                        }
                    }

                    // Create UEF file
                    string UEF_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tape_file, "uef");
                    if (!UEF_encoder.encode(tape_file, UEF_file_name)) {
                        cout << "Failed to write the UEF file!\n";
                        //return -1;
                    }

                    // Create BIN file
                    string BIN_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tape_file, "bin");
                    if (!TAPCodec::data2Binary(tape_file, BIN_file_name)) {
                        cout << "can't create Binary file " << BIN_file_name << "\n";
                        //return -1;
                    }

                }

                else if (!genTapeFile && arg_parser.cat) {

                    // Log found file
                    tape_file.logTAPFileHdr();
                }

                else if (genTapeFile) {

                    // Add program to UEF/CSW/WAV tape file

                    if (arg_parser.genUEF) {
                        if (!UEF_encoder.encode(tape_file)) {
                            cout << "Failed to update the UEF file!\n";
                            UEF_encoder.closeTapeFile();
                            return -1;
                        }
                    }
                    else if (arg_parser.genCSW) {
                        if (!CSW_encoder.encode(tape_file)) {
                            cout << "Failed to update the CSW file!\n";
                            CSW_encoder.closeTapeFile();
                            return -1;
                        }
                    }
                    else if (arg_parser.genWAV) {
                        if (!WAV_encoder.encode(tape_file)) {
                            cout << "Failed to update the WAV file!\n";
                            WAV_encoder.closeTapeFile();
                            return -1;
                        }
                    }
                    else {
                        return (-1);
                    }
                }
            }
        }
    }
    else {
        DiscCodec DISC_codec = DiscCodec(arg_parser.logging);
        filesystem::path file_path = arg_parser.srcFileName;
        string title = Utility::crReadableString(file_path.stem().string(), 12);
        if (arg_parser.searchedProgram == "") {
            if (!DISC_codec.write(title, arg_parser.dstFileName, tape_files)) {
                cout << "Failed to create disc image!\n";
                return -1;
            }
        }
        else {
            vector <TapeFile> selected_tape_files;
            for (int i = 0; i < tape_files.size(); i++) {
                TapeFile &tape_file = tape_files[i];
                if (tape_file.programName == arg_parser.searchedProgram) {
                    selected_tape_files.push_back(tape_file);
                    selected_file_found = tape_file.programName == arg_parser.searchedProgram;
                }
            }
            if (selected_file_found && !DISC_codec.write(title, arg_parser.dstFileName, selected_tape_files)) {
                cout << "Failed to create disc image!\n";
                return -1;
            }
        }
    }

    if (arg_parser.searchedProgram != "" && !selected_file_found)
        cout << "Couldn't find tape file '" << arg_parser.searchedProgram << "'!\n";
    else if (!arg_parser.cat && arg_parser.searchedProgram != "")
        cout << "Found tape file '" << arg_parser.searchedProgram << "'; output file(s) generated!\n";

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
    return 0;

}



