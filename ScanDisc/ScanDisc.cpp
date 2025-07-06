
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
#include "../shared/CSWCodec.h"
#include "../shared/WavEncoder.h"
#include "../shared/DiscCodec.h"
#include "../shared/BinCodec.h"

using namespace std;
using namespace std::filesystem;



/*
 *
 * Decode an Acorn DFS disc file
 * *
 */
int main(int argc, const char* argv[])
{


    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    // Create a log file
    ostream* fout_p = &cout;
    if (!arg_parser.cat) {

        string fout_name = "tape.log";
        filesystem::path fout_d = arg_parser.dstDir;
        fout_d /= fout_name;
        string fout_path = fout_d.string();

        fout_p = new ofstream(fout_path);
        if (!*fout_p) {
            cout << "can't write to log file " << fout_name << "\n";
            return false;
        }
    }

    
    // Prepare encoders for later use
    UEFCodec UEF_encoder(false, arg_parser.logging, arg_parser.targetMachine);
    CSWCodec CSW_encoder(false, 44100, arg_parser.tapeTiming, arg_parser.logging, arg_parser.targetMachine);
    WavEncoder WAV_encoder(false, 44100, arg_parser.tapeTiming, arg_parser.logging, arg_parser.targetMachine);
    TAPCodec TAP_encoder(arg_parser.logging, arg_parser.targetMachine);

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

    // Scan Disc for  files
    DiscCodec DISC_codec = DiscCodec(arg_parser.logging);
    Disc disc;
    DISC_codec.read(arg_parser.srcFileName, disc);

    // Collect files from disc
    vector <TapeFile> tape_files;
    bool selected_file_found = false;
    for (int side = 0; side < disc.side.size(); side++) {

        for (int file_no = 0; file_no < disc.side[side].files.size(); file_no++) {

            // Decode one disc file into the internal Tape File format
            DiscFile& file = disc.side[side].files[file_no];
            BinCodec BIN_Codec(arg_parser.logging);
            TapeFile tape_file(arg_parser.targetMachine);
            FileHeader file_header(file.dir + "." + file.name, file.loadAdr, file.execAdr, file.size, arg_parser.targetMachine, file.locked);
            if (!BIN_Codec.decode(file_header, file.data, tape_file)) {
                cout << "Failed to decode disc file '" << file.name << "'\n";
                //return false;
            }
            else {
                // If the file was read with some content (even if there were some errors), then add it to the list of tape files
                if (
                    tape_file.blocks.size() > 0 &&
                    (arg_parser.searchedProgram == "" || tape_file.header.name == arg_parser.searchedProgram)
                )
                    tape_files.push_back(tape_file);
            }
            
        }
    }


    // Iterate over the collected files
    for (int i = 0; i < tape_files.size(); i++) {

        TapeFile& tape_file = tape_files[i];   

        if (!genTapeFile && !arg_parser.cat) {

            // Generate the different types of files (DATA, ABC/BBC, TAP, UEF, BIN) for the found file

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
            if (!ABC_codec.detokenise(tape_file, ABC_file_name)) {
                cout << "Failed to write the program file!\n";
                //return -1;
            }

            // Create TAP file
            TAPCodec TAP_codec = TAPCodec(arg_parser.logging);
            string TAP_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tape_file, "tap");
            if (!TAP_codec.encode(tape_file, TAP_file_name)) {
                cout << "Failed to write the TAP file!\n";
                //return -1;
            }


            // Create UEF file
            string UEF_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tape_file, "uef");
            if (!UEF_encoder.encode(tape_file, UEF_file_name)) {
                cout << "Failed to write the UEF file!\n";
                //return -1;
            }

            // Create BIN file
            string BIN_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tape_file, "");
            BinCodec BIN_codec(arg_parser.logging);
            if (!BIN_codec.encode(tape_file, BIN_file_name)) {
                cout << "can't create Binary file " << BIN_file_name << "\n";
                //return -1;
            }

            // Create INF file
            if (!BinCodec::generateInfFile(arg_parser.dstDir, tape_file)) {
                cout << "Failed to write the INF file!\n";
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
            else if (arg_parser.genTAP) {
                if (!TAP_encoder.encode(tape_file)) {
                    cout << "Failed to update the TAP file!\n";
                    WAV_encoder.closeTapeFile();
                    return -1;
                }
            }
            else {
                return (-1);
            }
        }

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

    // Close and delete log file - if applicable
    if (fout_p != &cout) {
        ((ofstream*)fout_p)->close();
        delete fout_p;
    }

    if (arg_parser.searchedProgram != "" && !selected_file_found)
        cout << "Couldn't find tape file '" << arg_parser.searchedProgram << "!'\n";

    return 0;

}



