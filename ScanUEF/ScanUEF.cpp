
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
#include "../shared/Debug.h"
#include "../shared/UEFCodec.h"
#include "../shared/AtomBasicCodec.h"
#include "../shared/DataCodec.h"
#include "../shared/PcmFile.h"
#include "../shared/Utility.h"
#include "../shared/MMCCodec.h"
#include "../shared/CSWCodec.h"

using namespace std;
using namespace std::filesystem;



/*
 *
 * Generates files from one UEF file containig more then one tape file
 * (The uef2x programs only extracts the first encountered tape file
 *  so ScanUEF complements this by extracting all of them.)
 */
int main(int argc, const char* argv[])
{


    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    if (arg_parser.verbose)
        cout << "Output dir = " << arg_parser.dstDir << "\n";

    UEFCodec UEF_codec = UEFCodec(arg_parser.verbose, arg_parser.targetMachine);

    // Scan UEF file for Tape Files
    vector<TapeFile> TAP_files;
    UEF_codec.decodeMultipleFiles(arg_parser.srcFileName, TAP_files, arg_parser.targetMachine);
    if (arg_parser.verbose)
        cout << "#UEF files = " << TAP_files.size() << "\n";

    // Generate separate files for each detected Tape File
    for (int i = 0; i < TAP_files.size(); i++) {

        TapeFile& tapFile = TAP_files[i];

        string tape_file_name = tapFile.blocks[0].blockName();

        if (tapFile.blocks.size() > 0) {

            if (arg_parser.verbose)
                cout << "Tape File '" << tapFile.blocks.front().atomHdr.name <<
                "' read. Base file name used for generated files is: '" << tapFile.validFileName << "'.\n";

            DataCodec DATA_codec = DataCodec(arg_parser.verbose);
            string DATA_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tapFile, "dat");
            if (!DATA_codec.encode(tapFile, DATA_file_name)) {
                cout << "Failed to write the DATA file!\n";
                //return -1;
            }

            AtomBasicCodec ABC_codec = AtomBasicCodec(arg_parser.verbose, arg_parser.targetMachine);
            string ABC_file_name;
            if (arg_parser.targetMachine <= BBC_MASTER)
                ABC_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tapFile, "bbc");
            else
                ABC_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tapFile, "abc");
            if (!ABC_codec.encode(tapFile, ABC_file_name)) {
                cout << "Failed to write the program file!\n";
                //return -1;
            }

            if (tapFile.complete) {

                // Only generate files if the Tape file was completed (without missing blocks)

                // Generate TAP file - only for Acorn Atom
                if (arg_parser.targetMachine == ACORN_ATOM) {
                    string TAP_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tapFile, "tap");
                    TAPCodec TAP_codec = TAPCodec(arg_parser.verbose);
                    if (!TAP_codec.encode(tapFile, TAP_file_name)) {
                        cout << "Failed to write TAP file '" << TAP_file_name << "'!\n";
                    }
                }

                // Get data bytes
                Bytes data;
                uint32_t load_adr;
                if (!TAPCodec::tap2Bytes(tapFile, load_adr, data)) {
                    printf("Failed to decode Tape file '%s'\n", tape_file_name.c_str());
                }

                BytesIter data_iter = data.begin();

                // Generate binary program file from data bytes
                string BIN_file_name = Utility::crEncodedFileNamefromDir(arg_parser.dstDir, tapFile, "");
                ofstream fout(BIN_file_name, ios::out | ios::binary | ios::ate);
                if (!fout) {
                    cout << "can't write to file " << BIN_file_name << "\n";
                    return false;
                }

                while (data_iter < data.end()) {
                    fout << *data_iter++;
                }
                fout.close();

            }

        }

    }
    return 0;

}



