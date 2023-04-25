#include "FileDecoder.h"
#include "BlockDecoder.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include "../shared/Debug.h"
#include "../shared/AtomBasicCodec.h"
#include "../shared/UEFCodec.h"
#include "../shared/TAPCodec.h"
#include "../shared/Utility.h"

namespace fs = std::filesystem;

using namespace std;




FileDecoder::FileDecoder(
    BlockDecoder& blockDecoder, ArgParser &argParser
): mBlockDecoder(blockDecoder), mArgParser(argParser)
{
    mTracing = argParser.tracing;
}


string timeToStr(double t) {
    char t_str[64];
    int t_h = (int) trunc(t / 3600);
    int t_m = (int) trunc((t - t_h) / 60);
    double t_s = t - t_h * 3600 - t_m * 60;
    sprintf(t_str, "%2d:%2d:%9.6f (%12f)", t_h, t_m, t_s, t);
    return string(t_str);
}

bool FileDecoder::readFile(ofstream &logFile)
{
    bool last_block = false;
    bool first_block = true;
    int load_adr_start = 0, load_adr_end = 0, exec_adr_start = 0, next_load_adr = 0;
    string file_name = "???";

    double tape_start_time = -1, tape_end_time = -1;
    bool failed = false;
    char s[256];
    int file_sz = 0;

    mTapFile.blocks.clear();
    mTapFile.complete = false;
    mTapFile.firstBlock = -1;
    mTapFile.lastBlock = -1;
    mTapFile.validFileName = "FAILED";

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
    BlockType last_valid_block_type = BlockType::Other;
    int last_valid_block_start_time = -1;
    int last_valid_block_end_time = -1;

 

    double min_lead_tone_duration = mArgParser.tapeTiming.minBlockTiming.firstBlockLeadToneDuration;

    // Read all blocks belonging to the same file
    while (!last_block) {

        ATMBlock read_block;
        BlockType block_type;
        int block_sz;
        bool isAbcProgram;
        int exec_adr, load_adr, load_adr_UB;
        string fn;
        
        // Read tape block as ATM block
        double block_start_time = mBlockDecoder.getTimeNum();
        mBlockDecoder.checkpoint();
        bool lead_tone_detected;
        bool success = mBlockDecoder.readBlock(
            min_lead_tone_duration, mArgParser.tapeTiming.minBlockTiming.trailerToneDuration, read_block, block_type, block_no,
            lead_tone_detected);


        // If no lead tone was detected it must be the end of the tape
        if (!lead_tone_detected) {
            break;
        }

        // Extract ATM header parameters
        bool complete_header = extractBlockPars(
            read_block, block_type, mBlockDecoder.nReadBytes,
            load_adr, load_adr_UB, exec_adr, block_sz, isAbcProgram, fn
        );
        if (!complete_header) {
            if (mTracing)
                DBG_PRINT(ERR, "Incomplete Atom Tape Block '%s' - failed to extract all block parameters\n", fn.c_str());
        }

        // End if the read block is part of another Atom Tape File
        if (fn != "???" && !first_block && fn != file_name) {
            if (mTracing)
                DBG_PRINT(ERR, "Block from another file '%s' encountered while reading file '%s'!\n", fn.c_str(), file_name.c_str());
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

                if (mTracing) {

                    string block_no_s = "#" + to_string(block_no);
                    string block_type_s = _BLOCK_ORDER(block_type);

                    DBG_PRINT(
                        ERR,
                        "Only correctly read %d bytes of file '%s': block %s (%s)!\n",
                        mBlockDecoder.nReadBytes, fn.c_str(), block_no_s.c_str(), block_type_s.c_str()
                    );
                }


                // Fill incompleted part of block with zeros...
                for (int i = 0; i < block_sz - mBlockDecoder.nReadBytes; i++)
                    read_block.data.push_back(0x0);

            }

            last_valid_block_no = block_no;
            last_valid_load_adr = load_adr;
            last_valid_load_adr_UB = load_adr_UB;
            last_valid_exec_adr = exec_adr;
            last_valid_block_sz = block_sz;
            last_valid_block_type = block_type;
            last_valid_block_start_time = block_start_time; 

            double block_end_time = mBlockDecoder.getTimeNum();
            last_valid_block_end_time = block_end_time;


            min_lead_tone_duration = mArgParser.tapeTiming.minBlockTiming.otherBlockLeadToneDuration;
            

            read_block.tapeStartTime = block_start_time;
            read_block.tapeEndTime = block_end_time;

            file_sz += block_sz;

            if (block_type & BlockType::Last)
                last_block = true;

            mTapFile.isAbcProgram = isAbcProgram;

            mTapFile.blocks.push_back(read_block);

            if (block_no != expected_block_no)
                missing_block = true;

            if (first_block) {

                file_name = fn;

                first_block_no = block_no;
                if (block_type & BlockType::First) {
                    first_block_found = true;
                    corrupted_block = false;
                    if (block_no != 0 && mTracing)
                        DBG_PRINT(ERR, "First block of %s with non-zero block no %d and start address %.4x!\n", read_block.hdr.name, block_no, load_adr);
                }
                first_block = false;
                load_adr_start = load_adr;
                exec_adr_start = exec_adr;
                mTapFile.validFileName = filenameFromBlockName(file_name);
                tape_start_time = block_start_time;
            }
            else {
                if (load_adr != next_load_adr && mTracing) {
                    DBG_PRINT(
                        ERR, "Load address of block #%d (0x%.4x) not a continuation of previous block (0x%.4x) as expected!\n",
                        block_no, load_adr, next_load_adr
                    );
                 }               
            }
            if (last_block) {

                load_adr_end = load_adr_UB;
                last_block_found = true;
            }

            tape_end_time = block_end_time;

            next_load_adr = load_adr_UB;

            
            sprintf(
                s, "%15s %4x %4x %4x %2d %6s %3d %32s %32s\n", file_name.c_str(),
                last_valid_load_adr, last_valid_load_adr_UB, last_valid_exec_adr, last_valid_block_no, _BLOCK_ORDER(last_valid_block_type), last_valid_block_sz,
                timeToStr(last_valid_block_start_time).c_str(), timeToStr(last_valid_block_end_time).c_str()
            );
            logFile <<  s;
            if (mTracing)
                cout << s;
            
        }


        if (block_no != -1) {
            expected_block_no = block_no + 1;
            last_block_no = block_no;
        }
        else {
            // If a corrupted block without a valid header was received, then 
            // we still guestimate the expected next block no and load address
            if (block_no != -1)
                expected_block_no = block_no + 1;
            else
                expected_block_no = 1;
            if (next_load_adr != 0)
                next_load_adr += 256;
            else
                next_load_adr = 0x2a00;
        }




    }

    if (file_name != "???") {


        if (!first_block_found || !last_block_found || missing_block || corrupted_block) {
            mTapFile.complete = false;
            DBG_PRINT(ERR, "At least one block missing or corrupted for file '%s'\n", mTapFile.validFileName.c_str());
            logFile << "*** ERR *** At least one block missing or corrupted for file '" << mTapFile.validFileName << "'\n";
        }
        else
            mTapFile.complete = true;

        mTapFile.firstBlock = first_block_no;
        mTapFile.lastBlock = last_block_no;

        mTapFile.validTiming = true;

        mTapFile.baudRate = mArgParser.tapeTiming.baudRate;
 
        sprintf(s, "\n%15s %4x %4x %4x %2d %5d %32s %32s\n\n", file_name.c_str(),
            load_adr_start, load_adr_end, exec_adr_start, last_block_no+1, file_sz,
            timeToStr(tape_start_time).c_str(), timeToStr(tape_end_time).c_str()
        );
        logFile << s;
        cout << s;


        return true;
    }
    else
        return false;

}




bool FileDecoder::getTAPFile(TAPFile &tapFile)
{
    tapFile = mTapFile;

    return true;
}
