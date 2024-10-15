#include "FileDecoder.h"
#include "BlockDecoder.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include "Logging.h"
#include "AtomBasicCodec.h"
#include "UEFCodec.h"
#include "TAPCodec.h"
#include "Utility.h"

namespace fs = std::filesystem;

using namespace std;




FileDecoder::FileDecoder(
    BlockDecoder& blockDecoder,
    Logging logging, TargetMachine targetMachine, TapeProperties tapeTiming, bool catOnly
) : mDebugInfo(logging), mBlockDecoder(blockDecoder), mCat(catOnly), mTarget(targetMachine), mTapeTiming(tapeTiming)
{
}


string FileDecoder::timeToStr(double t) {
    char t_str[64];
    int t_h = (int)trunc(t / 3600);
    int t_m = (int)trunc((t - t_h) / 60);
    double t_s = t - t_h * 3600 - t_m * 60;
    sprintf(t_str, "%2d:%2d:%9.6f (%12f)", t_h, t_m, t_s, t);
    return string(t_str);
}

bool FileDecoder::readFile(ostream& logFile, TapeFile& tapFile, string searchName)
{
    tapFile.init();
    FileMetaData meta_data("", 0x0, 0x0, mTarget);
    tapFile.metaData = meta_data;

    bool file_selected = false;

    bool last_block = false;
    bool first_block = true;
    int load_adr_start = 0, load_adr_end = 0, exec_adr_start = 0, next_load_adr = 0;
    string file_name = "???";

    double tape_start_time = -1, tape_end_time = -1;
    bool failed = false;
    int file_sz = 0;

    int expected_block_no = 0;
    bool first_block_found = false;
    bool missing_block = false;
    bool corrupted_blocks = false;
    bool last_block_found = false;
    bool incomplete_blocks = false;

    int first_block_no = -1;
    int last_block_no = -1;
    int block_no = -1;

    int last_valid_block_no = -1;
    int last_valid_load_adr = -1;
    int last_valid_load_adr_UB = -1;
    int last_valid_exec_adr = -1;
    int last_valid_block_sz = -1;
    BlockType last_valid_block_type = BlockType::Other;
    double last_valid_block_start_time = -1;
    double last_valid_block_end_time = -1;

    BlockTiming blockTiming = mTapeTiming.minBlockTiming;

    // Read all blocks belonging to the same file
    unsigned n_blocks = 0;
    uint32_t adr_offset = 0;
    while (!last_block) {

        bool corrupted_block = false;
        bool incomplete_block = false;

        FileBlock read_block(mTarget);
        int block_sz;
        bool isBasicProgram;
        int exec_adr, load_adr, load_adr_UB, load_adr_LB;
        string fn;

        // Save tape time when block starts
        double block_start_time = mBlockDecoder.getTime();

        // Save tape position in case the block coming up would turn out not be part
        // of the tape file currently being read. In that case, a rollback to this
        // saved position will be made an the reading of the tape file will stop.
        mBlockDecoder.checkpoint();

        // Read one block
        bool lead_tone_detected;  
        BlockError block_error;
        bool success = mBlockDecoder.readBlock(blockTiming, first_block, read_block, lead_tone_detected, block_error);
        block_no = read_block.blockNo;



        // If no lead tone was detected it must be the end of the tape
        if (!lead_tone_detected) {
            break;
        }

        // Extract ATM/BTM header parameters
        int expected_bytes_to_read;
        if (mTarget <= BBC_MASTER)
            expected_bytes_to_read = (int) read_block.blockName().length() + 1 + read_block.tapeHdrSz() + 2 + read_block.dataSz() + 2;
        else
            expected_bytes_to_read = (int) read_block.blockName().length() + 1 + read_block.tapeHdrSz() + read_block.dataSz() + 1;
        bool complete_header = (mBlockDecoder.nReadBytes == expected_bytes_to_read);
        load_adr = read_block.loadAdr();
        exec_adr = read_block.execAdr();
        block_sz = read_block.dataSz();
        fn = read_block.blockName();
        isBasicProgram = read_block.isBasicProgram();

        if (mTarget == ACORN_ATOM) {
            load_adr_LB = load_adr;
            load_adr_UB = load_adr + block_sz - 1;
        }
        else {
            load_adr_LB = load_adr + adr_offset;
            load_adr_UB = load_adr + adr_offset + block_sz - 1;
        }

        if (!complete_header) {
            if (mDebugInfo.verbose)
                printf(
                    "*** ERROR *** Incomplete Tape Header Block '%s' - only read %d out of %d bytes!\n", fn.c_str(),
                    mBlockDecoder.nReadBytes, expected_bytes_to_read);
        }

        // End if the read block is part of another Tape File => rollback to tape position
        // prior to the reading of the block and exit.
        if (fn != "???" && !first_block && fn != file_name) {
            if (mDebugInfo.verbose)
                printf("*** ERROR *** Block from another file '%s' encountered while reading file '%s'!\n", fn.c_str(), file_name.c_str());
            mBlockDecoder.rollback();
            break;
        }

        if (!complete_header || mBlockDecoder.nReadBytes < block_sz) {
            incomplete_block = true;
            incomplete_blocks = true;
        }

        if ((block_error & BLOCK_CRC_ERR) != 0) {
            corrupted_block = true;
            corrupted_blocks = true;
        }

        file_selected = (searchName == "" || fn == searchName);
 
        // Store read block if it contains a valid header
        // Also an only partially read block will be recovered by replacing
        // the missing bytes with zeroes.
        // (This means than if only a valid header was read but no data at all,
        //  then an all-zero data part will still be created!)
        if (complete_header) {

            file_selected = file_selected || (searchName == "" || fn == searchName);
 
            // Recover data of any incomplete data block (by replacing it with zeros)
            if (mBlockDecoder.nReadBytes < block_sz) {

                if (mDebugInfo.verbose && file_selected) {

                    string block_no_s = "#" + to_string(block_no);
                    string block_type_s = _BLOCK_ORDER(read_block.getBlockType());

                    printf("*** ERROR *** Only correctly read %d bytes of file '%s': block %s (%s)!\n",
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
            last_valid_block_type = read_block.getBlockType();
            last_valid_block_start_time = block_start_time;

            double block_end_time = mBlockDecoder.getTime();
            last_valid_block_end_time = block_end_time;

            read_block.tapeStartTime = block_start_time;
            read_block.tapeEndTime = block_end_time;

            file_sz += block_sz;

            last_block = read_block.lastBlock();

            tapFile.isBasicProgram = isBasicProgram;

            tapFile.blocks.push_back(read_block);
            n_blocks++;

            if (block_no != expected_block_no) {
                if (mDebugInfo.verbose)
                    cout << "*** ERROR *** Read block no " << block_no << " different than the expected " << expected_block_no << "!\n";
                missing_block = true;
            }

            if (first_block) {

                file_name = fn;

                first_block_no = block_no;
                if (read_block.firstBlock()) {
                    first_block_found = true;
                    incomplete_blocks = false;
                    if (block_no != 0 && mDebugInfo.verbose && file_selected)
                        printf("First block of %s with non-zero block no %d and start address %.4x!\n",
                            read_block.blockName().c_str(), block_no, load_adr);
                }
                first_block = false;
                load_adr_start = load_adr;
                exec_adr_start = exec_adr;
                tapFile.validFileName = read_block.filenameFromBlockName(file_name);
                tape_start_time = block_start_time;
            }
            else {
                if (load_adr_LB != next_load_adr && mDebugInfo.verbose && file_selected) {
                    printf("*** ERROR *** Load address of block #%d (0x%.4x) not a continuation of previous block (0x%.4x) as expected!\n",
                        block_no, load_adr_LB, next_load_adr
                    );
                }
            }
            if (last_block) {

                load_adr_end = load_adr_UB;
                last_block_found = true;
            }

            tape_end_time = block_end_time;

            next_load_adr = load_adr_UB+1;

            // Predict next block no
            expected_block_no = block_no + 1;

            last_block_no = block_no;

            if (incomplete_block || corrupted_block) {
                logFile << "*";
                if (mDebugInfo.verbose)
                    cout << "*";
            }
            if (file_selected && !mCat)
                read_block.logHdr(&logFile);

            if (mDebugInfo.verbose && file_selected)
                read_block.logHdr();


            adr_offset += block_sz;

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

        if (!first_block_found || !last_block_found || missing_block || incomplete_blocks || corrupted_blocks) {
            if (!first_block_found || !last_block_found || missing_block || incomplete_blocks)
                tapFile.complete = false;
            else
                tapFile.complete = true;
            if (corrupted_blocks)
                tapFile.corrupted = true;
            if (file_selected && !mCat) {
                printf(
                    "At least one block missing or corrupted for file '%s' [%s,%s]\n",
                    tapFile.validFileName.c_str(),
                    Utility::encodeTime(tapFile.blocks[0].tapeStartTime).c_str(),
                    Utility::encodeTime(tapFile.blocks[n_blocks - 1].tapeEndTime).c_str()
                );
                logFile << "*** ERR *** At least one block missing or corrupted for file '" << tapFile.validFileName << "' [";
                logFile << Utility::encodeTime(tapFile.blocks[0].tapeStartTime) << "," << Utility::encodeTime(tapFile.blocks[n_blocks - 1].tapeEndTime) << "]\n";
            }
        }
        else
            tapFile.complete = true;

        tapFile.firstBlock = first_block_no;
        tapFile.lastBlock = last_block_no;

        tapFile.validTiming = true;

        tapFile.baudRate = mTapeTiming.baudRate;

        if (file_selected && !mCat) {
            logFile << "\n";
            tapFile.logTAPFileHdr(&logFile);
            logFile << "\n\n";
        }

        if (mDebugInfo.verbose && file_selected) {
            cout << "\n";
            tapFile.logTAPFileHdr();
            cout << "\n\n";
        }


        return true;
    }
    else
        return false;

}
