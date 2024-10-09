
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
    vector<TapeFile> TAP_files;
    TAP_codec.decodeMultipleFiles(arg_parser.srcFileName, TAP_files);
    if (arg_parser.logging.verbose)
        cout << "#TAP files = " << TAP_files.size() << "\n";

    // Generate separate files for each detected Atom file
    for (int i = 0; i < TAP_files.size(); i++) {

        TapeFile& tape_file = TAP_files[i];

        if (
            !genTapeFile && !arg_parser.cat &&
            tape_file.blocks.size() > 0 &&
            (arg_parser.find_file_name == "" || tape_file.blocks[0].blockName() == arg_parser.find_file_name)
            ) {

            if (arg_parser.logging.verbose)
                cout << "Atom Tape File '" << tape_file.blocks.front().atomHdr.name <<
                "' read. Base file name used for generated files is: '" << tape_file.validFileName << "'.\n";

            DataCodec DATA_codec = DataCodec(arg_parser.logging);
            string DATA_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tape_file, "dat");
            if (!DATA_codec.encode(tape_file, DATA_file_name)) {
                cout << "Failed to write the DATA file!\n";
                //return -1;
            }

            AtomBasicCodec ABC_codec = AtomBasicCodec(arg_parser.logging, ACORN_ATOM);
            string ABC_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tape_file, "abc");
            if (!ABC_codec.encode(tape_file, ABC_file_name)) {
                cout << "Failed to write the program file!\n";
                //return -1;
            }

            if (tape_file.complete) {

                // Only generate TAP & UEF files if the Tape file was completed (without missing blocks)

                TAPCodec TAP_codec = TAPCodec(arg_parser.logging);
                string TAP_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tape_file, "");
                if (!TAP_codec.encode(tape_file, TAP_file_name)) {
                    cout << "Failed to write the TAP file!\n";
                    //return -1;
                }

                UEFCodec UEF_codec = UEFCodec(false, arg_parser.logging, ACORN_ATOM);
                string UEF_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tape_file, "uef");
                if (!UEF_codec.encode(tape_file, UEF_file_name)) {
                    cout << "Failed to write the UEF file!\n";
                    //return -1;
                }

            }

        }

        else if (!genTapeFile && arg_parser.cat && tape_file.blocks.size() > 0) {
            tape_file.logTAPFileHdr();
        }
        else if (genTapeFile) {
                if (tape_file.complete && !tape_file.corrupted) {
                    // Only include program if it was successfully decoded   
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



