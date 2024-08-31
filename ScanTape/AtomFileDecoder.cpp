#include "AtomFileDecoder.h"
#include "AtomBlockDecoder.h"
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




AtomFileDecoder::AtomFileDecoder(
    AtomBlockDecoder& blockDecoder, ArgParser &argParser
): FileDecoder(argParser), mBlockDecoder(blockDecoder)
{
}


bool AtomFileDecoder::readFile(ofstream &logFile, TapeFile &tapFile)
{
    TargetMachine file_block_type = ACORN_ATOM;
    bool last_block = false;
    bool first_block = true;
    int load_adr_start = 0, load_adr_end = 0, exec_adr_start = 0, next_load_adr = 0;
    string file_name = "???";

    double tape_start_time = -1, tape_end_time = -1;
    bool failed = false;
    //char s[256];
    int file_sz = 0;

    tapFile.blocks.clear();
    tapFile.complete = false;
    tapFile.firstBlock = -1;
    tapFile.lastBlock = -1;
    tapFile.validFileName = "FAILED";

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
    unsigned n_blocks = 0;
    uint32_t adr_offset = 0;
    while (!last_block) {

        FileBlock read_block(ACORN_ATOM);
        BlockType block_type;
        int block_sz;
        bool isBasicProgram;
        int exec_adr, load_adr, load_adr_UB;
        string fn;
        
        // Read tape block as ATM block
        double block_start_time = mBlockDecoder.getTime();
        mBlockDecoder.checkpoint();
        bool lead_tone_detected;
        bool success = mBlockDecoder.readBlock(
            min_lead_tone_duration, read_block, block_type, block_no,
            lead_tone_detected
        );


        // If no lead tone was detected it must be the end of the tape
        if (!lead_tone_detected) {
            break;
        }

        // Extract ATM header parameters
        bool complete_header = Utility::decodeFileBlockHdr(
            file_block_type, read_block, mBlockDecoder.nReadBytes,
            load_adr, exec_adr, block_sz, isBasicProgram, fn
        );
        load_adr_UB = load_adr + block_sz - 1;
        if (!complete_header) {
            if (mVerbose)
                printf("Incomplete Atom Tape Block '%s' - failed to extract all block parameters\n", fn.c_str());
        }

        // End if the read block is part of another Atom Tape File
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
                    string block_type_s = _BLOCK_ORDER(block_type);

                    printf("Only correctly read %d bytes of file '%s': block %s (%s)!\n",
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

            double block_end_time = mBlockDecoder.getTime();
            last_valid_block_end_time = block_end_time;


            min_lead_tone_duration = mArgParser.tapeTiming.minBlockTiming.otherBlockLeadToneDuration;
            

            read_block.tapeStartTime = block_start_time;
            read_block.tapeEndTime = block_end_time;

            file_sz += block_sz;

            if (block_type & BlockType::Last)
                last_block = true;

            tapFile.isBasicProgram = isBasicProgram;

            tapFile.blocks.push_back(read_block);
            n_blocks++;

            if (block_no != expected_block_no)
                missing_block = true;

            if (first_block) {

                file_name = fn;

                first_block_no = block_no;
                if (block_type & BlockType::First) {
                    first_block_found = true;
                    corrupted_block = false;
                    if (block_no != 0 && mVerbose)
                        printf("First block of %s with non-zero block no %d and start address %.4x!\n", read_block.atomHdr.name, block_no, load_adr);
                }
                first_block = false;
                load_adr_start = load_adr;
                exec_adr_start = exec_adr;
                tapFile.validFileName = Utility::filenameFromBlockName(file_name);
                tape_start_time = block_start_time;
            }
            else {
                if (load_adr != next_load_adr && mVerbose) {
                    printf("Load address of block #%d (0x%.4x) not a continuation of previous block (0x%.4x) as expected!\n",
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

            // Predict next block no
            expected_block_no = block_no + 1;

            last_block_no = block_no;

            Utility::logTAPBlockHdr(&logFile, read_block, 0x0, last_valid_block_no);
            if (mArgParser.verbose)
                Utility::logTAPBlockHdr(read_block, adr_offset, last_valid_block_no);

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


        if (!first_block_found || !last_block_found || missing_block || corrupted_block) {
            tapFile.complete = false;
            printf(
                "At least one block missing or corrupted for file '%s' [%s,%s]\n",
                tapFile.validFileName.c_str(),
                Utility::encodeTime(tapFile.blocks[0].tapeStartTime).c_str(),
                Utility::encodeTime(tapFile.blocks[n_blocks-1].tapeEndTime).c_str()
            );
            logFile << "*** ERR *** At least one block missing or corrupted for file '" << tapFile.validFileName << "' [";
            logFile << Utility::encodeTime(tapFile.blocks[0].tapeStartTime) << "," << Utility::encodeTime(tapFile.blocks[n_blocks - 1].tapeEndTime) << "]\n";
        }
        else
            tapFile.complete = true;

        tapFile.firstBlock = first_block_no;
        tapFile.lastBlock = last_block_no;

        tapFile.validTiming = true;

        tapFile.baudRate = mArgParser.tapeTiming.baudRate;
 
        Utility::logTAPFileHdr(&logFile, tapFile);
        if (mArgParser.verbose)
            Utility::logTAPFileHdr(tapFile);


        return true;
    }
    else
        return false;

}
