
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

    // Scan Disc for  files
    DiscCodec DISC_codec = DiscCodec(arg_parser.logging);
    Disc disc;
    DISC_codec.read(arg_parser.srcFileName, disc);

    // Generate separate files for each detected  file
    bool selected_file_found = false;
    for (int side = 0; side < disc.side.size(); side++) {

        for (int file_no = 0; file_no < disc.side[side].files.size(); file_no++) {

            // Decode one disc file into the internal Tape File format
            DiscFile &file = disc.side[side].files[file_no];
            BinCodec BIN_Codec(arg_parser.logging);
            TapeFile tape_file(arg_parser.targetMachine);
            FileMetaData file_meta_data(file.name, file.execAdr, file.loadAdr, arg_parser.targetMachine);
            if (!BIN_Codec.decode(file_meta_data, file.data, tape_file)) {
                cout << "Failed to decode disc file '" << file.name << "'\n";
                return false;
            }


            if (
                !genTapeFile && !arg_parser.cat &&
                tape_file.blocks.size() > 0 &&
                (arg_parser.find_file_name == "" || tape_file.blocks[0].blockName() == arg_parser.find_file_name)
                ) {

                selected_file_found = true;

                if (arg_parser.logging.verbose || tape_file.blocks[0].blockName() == arg_parser.find_file_name)
                    tape_file.logTAPFileHdr();

                tape_file.logTAPFileHdr(fout_p);
                *fout_p << "\n";

                if (arg_parser.logging.verbose)
                    cout << "Atom Tape File '" << tape_file.blocks.front().atomHdr.name <<
                    "' read. Base file name used for generated files is: '" << tape_file.validFileName << "'.\n";

                DataCodec DATA_codec = DataCodec(arg_parser.logging);
                string DATA_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tape_file, "dat");
                if (!DATA_codec.encode(tape_file, DATA_file_name)) {
                    cout << "Failed to write the DATA file!\n";
                    //return -1;
                }

                AtomBasicCodec ABC_codec = AtomBasicCodec(arg_parser.logging, arg_parser.targetMachine);
                string ABC_file_name;
                if (tape_file.metaData.targetMachine == ACORN_ATOM)
                    ABC_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tape_file, "abc");
                else if (tape_file.metaData.targetMachine <= BBC_MASTER)
                    ABC_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tape_file, "bbc");
                else {
                    cout << "Unknwon target machine " << hex << tape_file.metaData.targetMachine << "\n";
                    return -1;
                };
                if (!ABC_codec.encode(tape_file, ABC_file_name)) {
                    cout << "Failed to write the program file!\n";
                    //return -1;
                }

                if (tape_file.complete) {

                    // Only generate TAP & UEF files if the Tape file was completed (without missing blocks)

                    // TAP is only for Acorn Atom
                    if (arg_parser.targetMachine == ACORN_ATOM) {
                        TAPCodec TAP_codec = TAPCodec(arg_parser.logging);
                        string TAP_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tape_file, "");
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

                    // Create binary file
                    string BIN_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tape_file, "");
                    if (!TAPCodec::data2Binary(tape_file, BIN_file_name)) {
                        cout << "can't create Binary file " << BIN_file_name << "\n";
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

    // Close and delete log file - if applicable
    if (fout_p != &cout) {
        ((ofstream*)fout_p)->close();
        delete fout_p;
    }

    if (arg_parser.find_file_name != "" && !selected_file_found)
        cout << "Couldn't find tape file '" << arg_parser.find_file_name << "!'\n";

    return 0;

}



