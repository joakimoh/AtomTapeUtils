
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
 * Create DATA file from TAP file
 * *
 */
int main(int argc, const char* argv[])
{


    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    cout << "Output dir = " << arg_parser.dstDir << "\n";

    TAPCodec TAP_codec = TAPCodec(arg_parser.verbose);

    // Scan TAP file for Atom files
    vector<TAPFile> TAP_files;
    TAP_codec.decodeMultipleFiles(arg_parser.srcFileName, TAP_files);
    cout << "#TAP files = " << TAP_files.size() << "\n";

    // Generate separate files for each detected Atom file
    for (int i = 0; i < TAP_files.size(); i++) {

        TAPFile& tapFile = TAP_files[i];

        if (tapFile.blocks.size() > 0) {

            if (arg_parser.verbose)
                cout << "Atom Tape File '" << tapFile.blocks.front().hdr.name <<
                "' read. Base file name used for generated files is: '" << tapFile.validFileName << "'.\n";

            DataCodec DATA_codec = DataCodec(tapFile, arg_parser.verbose);
            string DATA_file_name = crEncodedFileNamefromDir(arg_parser.dstDir, tapFile, "dat");
            if (!DATA_codec.encode(DATA_file_name)) {
                cout << "Failed to write the DATA file!\n";
                //return -1;
            }

            AtomBasicCodec ABC_codec = AtomBasicCodec(tapFile, arg_parser.verbose);
            string ABC_file_name = crEncodedFileNamefromDir(arg_parser.dstDir, tapFile, "abc");
            if (!ABC_codec.encode(ABC_file_name)) {
                cout << "Failed to write the program file!\n";
                //return -1;
            }

            if (tapFile.complete) {

                // Only generate TAP & UEF files if the Tape file was completed (without missing blocks)

                TAPCodec TAP_codec = TAPCodec(tapFile, arg_parser.verbose);
                string TAP_file_name = crEncodedFileNamefromDir(arg_parser.dstDir, tapFile, "");
                if (!TAP_codec.encode(TAP_file_name)) {
                    cout << "Failed to write the TAP file!\n";
                    //return -1;
                }

                UEFCodec UEF_codec = UEFCodec(tapFile, false, arg_parser.verbose);
                string UEF_file_name = crEncodedFileNamefromDir(arg_parser.dstDir, tapFile, "uef");
                if (!UEF_codec.encode(UEF_file_name)) {
                    cout << "Failed to write the UEF file!\n";
                    //return -1;
                }

            }

        }

    }
    return 0;

}



