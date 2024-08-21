#include "BBMFileDecoder.h"
#include "BBMBlockDecoder.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include "../shared/Debug.h"
#include "../shared/Utility.h"

namespace fs = std::filesystem;

using namespace std;


BBMFileDecoder::BBMFileDecoder(
    BBMBlockDecoder& blockDecoder, ArgParser& argParser
    ) : FileDecoder(argParser), mBlockDecoder(blockDecoder)
{
}


bool BBMFileDecoder::readFile(ofstream& logFile, TapeFile &tapeFile)
{
    bool last_block = false;
    bool first_block = true;
    int load_adr_start = 0, load_adr_end = 0, exec_adr_start = 0, next_load_adr = 0;
    string file_name = "???";
    bool is_last_block = false;

    double tape_start_time = -1, tape_end_time = -1;
    bool failed = false;
    char s[256];
    int file_sz = 0;

    tapeFile.blocks.clear();
    tapeFile.complete = false;
    tapeFile.firstBlock = -1;
    tapeFile.lastBlock = -1;
    tapeFile.validFileName = "FAILED";

    int expected_block_no = 0;
    bool first_block_found = false;
    bool missing_block = false;
    bool last_block_found = false;
    bool corrupted_block = false;

    int first_block_no = -1;
    int last_block_no = -1;
    int block_no = -1;

    int last_valid_block_no = -1;
    int last_valid_load_adr = -1;
    int last_valid_load_adr_UB = -1;
    int last_valid_exec_adr = -1;
    int last_valid_block_sz = -1;
    bool last_valid_last_block_status = is_last_block;
    int last_valid_block_start_time = -1;
    int last_valid_block_end_time = -1;



    double min_lead_tone_duration = mArgParser.tapeTiming.minBlockTiming.firstBlockLeadToneDuration;
    double min_trailer_tone_duration = mArgParser.tapeTiming.minBlockTiming.trailerToneDuration;

    // Read all blocks belonging to the same file
    unsigned n_blocks = 0;
    while (!last_block) {

        FileBlock read_block(BBCMicroBlock);
        int block_sz;
        bool isBasicProgram;
        int exec_adr, file_load_adr, load_adr_UB;
        string fn;

        // Read tape block as BBM block
        double block_start_time = mBlockDecoder.getTime();
        mBlockDecoder.checkpoint();
        bool lead_tone_detected;
        bool success = mBlockDecoder.readBlock(
            min_lead_tone_duration, min_trailer_tone_duration, read_block, first_block, is_last_block, block_no,
            lead_tone_detected
        );


        // If no lead tone was detected it must be the end of the tape
        if (!lead_tone_detected) {
            break;
        }

        // Extract BBM header parameters
        bool complete_header = extractBBMBlockPars(
            read_block, is_last_block, mBlockDecoder.nReadBytes,
            file_load_adr, load_adr_UB, exec_adr, block_sz, isBasicProgram, fn
        );
        if (!complete_header) {
            if (mVerbose)
                printf("Incomplete BBC Micro Tape Block '%s' - failed to extract all block parameters\n", fn.c_str());
        }

        // End if the read block is part of another BBC Micro Tape File
        if (fn != "???" && !first_block && fn != file_name) {
            if (mVerbose)
                printf("Block from another file '%s' encountered while reading file '%s'!\n", fn.c_str(), file_name.c_str());
            mBlockDecoder.rollback();
            break;
        }

        if (!complete_header || mBlockDecoder.nReadBytes < block_sz) {
            corrupted_block = true;
        }

        // Store read block if it contains a valid header
        // Also an only partially read block will be recovered by replacing
        // the missing bytes with zeroes.
        // (This means than if only a valid header was read but no data at all,
        //  then an all-zero data part will still be created!)
        if (complete_header) {

            // Recover data of any incomplete data block (by replacing it with zeros)
            if (mBlockDecoder.nReadBytes < block_sz) {

                if (mVerbose) {

                    string block_no_s = "#" + to_string(block_no);
                    string block_type_s = _BBM_BLOCK_ORDER(is_last_block, block_no);

                    printf("Only correctly read %d (%d) bytes of file '%s': block %s (%s)!\n",
                        mBlockDecoder.nReadBytes, block_sz, fn.c_str(), block_no_s.c_str(), block_type_s.c_str()
                    );
                }


                // Fill incompleted part of block with zeros...
                for (int i = 0; i < block_sz - mBlockDecoder.nReadBytes; i++)
                    read_block.data.push_back(0x0);

            }

            last_valid_block_no = block_no;
            last_valid_load_adr = file_load_adr;
            last_valid_load_adr_UB = load_adr_UB;
            last_valid_exec_adr = exec_adr;
            last_valid_block_sz = block_sz;
            last_valid_last_block_status = is_last_block;
            last_valid_block_start_time = block_start_time;

            double block_end_time = mBlockDecoder.getTime();
            last_valid_block_end_time = block_end_time;


            min_lead_tone_duration = mArgParser.tapeTiming.minBlockTiming.otherBlockLeadToneDuration;


            read_block.tapeStartTime = block_start_time;
            read_block.tapeEndTime = block_end_time;

            file_sz += block_sz;

            if (is_last_block)
                last_block = true;

            tapeFile.isBasicProgram = isBasicProgram;

            tapeFile.blocks.push_back(read_block);
            n_blocks++;

            if (block_no != expected_block_no)
                missing_block = true;

            if (first_block) {

                file_name = fn;

                first_block_no = block_no;
                if (block_no == 0) {
                    first_block_found = true;
                    corrupted_block = false;
                    if (block_no != 0 && mVerbose)
                        printf("First block of %s with non-zero block no %d and start address %.4x!\n", read_block.bbmHdr.name, block_no, file_load_adr);
                }
                first_block = false;
                load_adr_start = file_load_adr;
                exec_adr_start = exec_adr;
                tapeFile.validFileName = filenameFromBlockName(file_name);
                tape_start_time = block_start_time;
            }
            if (last_block) {

                load_adr_end = load_adr_UB;
                last_block_found = true;
            }

            tape_end_time = block_end_time;

            next_load_adr = load_adr_UB;

            // Predict next block no
            expected_block_no = block_no + 1;

            last_block_no = block_no;


            sprintf(
                s, "%15s %4x %4x %4x %2d %6s %3d %32s %32s\n", file_name.c_str(),
                last_valid_load_adr, last_valid_load_adr_UB, last_valid_exec_adr, last_valid_block_no, 
                _BBM_BLOCK_ORDER(last_valid_last_block_status, last_valid_block_no), last_valid_block_sz,
                timeToStr(last_valid_block_start_time).c_str(), timeToStr(last_valid_block_end_time).c_str()
            );
            logFile << s;
            if (mArgParser.verbose)
                cout << s;

        }

        //
        // Predict the next block no and load address
        // when the header was not complete and at least one
        // valid block for the file has previously been detected.
        //
        if (!complete_header && n_blocks > 0) {
            expected_block_no = expected_block_no + 1;
            next_load_adr += 256;

        }




    }

    if (file_name != "???") {


        if (!first_block_found || !last_block_found || missing_block || corrupted_block) {
            tapeFile.complete = false;
            printf(
                "At least one block missing or corrupted for file '%s' [%s,%s]\n",
                tapeFile.validFileName.c_str(),
                encodeTime(tapeFile.blocks[0].tapeStartTime).c_str(),
                encodeTime(tapeFile.blocks[n_blocks - 1].tapeEndTime).c_str()
            );
            logFile << "*** ERR *** At least one block missing or corrupted for file '" << tapeFile.validFileName << "' [";
            logFile << encodeTime(tapeFile.blocks[0].tapeStartTime) << "," << encodeTime(tapeFile.blocks[n_blocks - 1].tapeEndTime) << "]\n";
        }
        else
            tapeFile.complete = true;

        tapeFile.firstBlock = first_block_no;
        tapeFile.lastBlock = last_block_no;

        tapeFile.validTiming = true;

        tapeFile.baudRate = mArgParser.tapeTiming.baudRate;

        sprintf(s, "\n%15s %4x %4x %4x %2d %5d %32s %32s\n\n", file_name.c_str(),
            load_adr_start, load_adr_end, exec_adr_start, last_block_no + 1, file_sz,
            timeToStr(tape_start_time).c_str(), timeToStr(tape_end_time).c_str()
        );
        logFile << s;
        if (mArgParser.verbose)
            cout << s;


        return true;
    }
    else
        return false;

}
