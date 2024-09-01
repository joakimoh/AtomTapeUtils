#include <gzstream.h>
#include <stdlib.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string.h>
#include "UEFCodec.h"
#include "CommonTypes.h"
#include "AtomBlockTypes.h"
#include "Debug.h"
#include "Utility.h"
#include "AtomBlockTypes.h"



using namespace std;

bool UEFCodec::decodeMultipleFiles(string& uefFileName, vector<TapeFile>& tapeFiles, TargetMachine targetMachine)
{
    if (mVerbose) {
        cout << "Decoding UEF files for target " << _TARGET_MACHINE(targetMachine) << "\n";
    }

    // Get data for all tape files
    Bytes data;
    if (!getUEFdata(uefFileName, data)) {
        cout << "Failed to parse UEF file '" << uefFileName << "'!\n";
        return false;
    }

    // Decode each of them
    BytesIter data_iter = data.begin();
    TapeFile tape_file(targetMachine);
    while (data_iter < data.end()) {
        if (!decodeFile(uefFileName, data, tape_file, data_iter)) {
            cout << "Failed to create Tape file '" << uefFileName << "'!\n";
            return false;
        }
        if (mVerbose) {
            cout << "Successfully decoded tape file '" << tape_file.blocks[0].blockName() << "' with #" << dec <<
                tape_file.blocks.size() << " blocks!\n";
        }
        tapeFiles.push_back(tape_file);
    }

    if (mVerbose)
        cout << "\nDone decoding TAP file '" << uefFileName << "'...\n\n";

    return true;
}



bool UEFCodec::decodeFloat(Byte encoded_val[4], float& decoded_val)
{

    /* decode mantissa */
    int m = encoded_val[0] | (encoded_val[1] << 8) | ((encoded_val[2] & 0x7f) | 0x80) << 16;

    decoded_val = (float) m;
    decoded_val = (float) ldexp(decoded_val, -23);

    /* decode exponent */
    int exp = (((encoded_val[2] & 0x80) >> 7) | (encoded_val[3] & 0x7f) << 1) - 127;
    decoded_val = (float) ldexp(decoded_val, exp);

    /* flip sign if necessary */
    if (encoded_val[3] & 0x80)
        decoded_val = -decoded_val;
  
    return true;
}

bool UEFCodec::encodeFloat(float val, Byte encoded_val[4])
{

    /* sign bit */
    if (val < 0)
    {
        val = -val;
        encoded_val[3] = 0x80;
    }
    else
        encoded_val[3] = 0;

    /* decode mantissa and exponent */
    int exp;
    float m = (float) frexp(val, &exp);
    exp += 126;

    /* store mantissa */
    uint32_t im = (uint32_t) (m * (1 << 24));
    encoded_val[0] = im & 0xff;
    encoded_val[1] = (im >> 8) & 0xff;
    encoded_val[2] = (im >> 16) & 0x7f;

    /* store exponent */
    encoded_val[3] |= exp >> 1;
    encoded_val[2] |= (exp & 1) << 7;

    return true;
}

UEFCodec::UEFCodec(bool verbose, TargetMachine mTargetMachine): mVerbose(verbose), mTargetMachine(mTargetMachine)
{
     if (mTargetMachine <= BBC_MASTER)
        mTapeTiming = bbmTiming;
    else if (mTargetMachine == ACORN_ATOM)
        mTapeTiming = atomTiming;
    else
        invalid_argument("Unsupported target machine " + mTargetMachine);
    if (mVerbose)
        Utility::logTapeTiming(mTapeTiming);

    mBaudRate = mTapeTiming.baudRate;
    mBaseFrequency = mTapeTiming.baseFreq;
    mPhase = mTapeTiming.phaseShift;
}

UEFCodec::UEFCodec(bool useOriginalTiming, bool verbose, TargetMachine mTargetMachine):
    mUseOriginalTiming(useOriginalTiming),mVerbose(verbose), mTargetMachine(mTargetMachine)
{
    if (mTargetMachine <= BBC_MASTER)
        mTapeTiming = bbmTiming;
    else if (mTargetMachine == ACORN_ATOM)
        mTapeTiming = atomTiming;
    else
        invalid_argument("Unsupported target machine " + mTargetMachine);
    if (mVerbose)
        Utility::logTapeTiming(mTapeTiming);

    mBaudRate = mTapeTiming.baudRate;
    mBaseFrequency = mTapeTiming.baseFreq;
    mPhase = mTapeTiming.phaseShift;
}


bool UEFCodec::setTapeTiming(TapeProperties tapeTiming)
{
    mTapeTiming = tapeTiming;

    mBaudRate = mTapeTiming.baudRate;
    mBaseFrequency = mTapeTiming.baseFreq;
    mPhase = mTapeTiming.phaseShift;

    return true;
}

bool UEFCodec::encodeBBM(TapeFile& tapeFile, ogzstream& fout)
{
    FileBlockIter file_block_iter = tapeFile.blocks.begin();
    TargetMachine file_block_type = BBC_MODEL_B;

    int block_no = 0;
    int n_blocks = (int)tapeFile.blocks.size();

    // Default values for block timing (if mUseOriginalTiming is true the recorded block timing will instead by used later on)
    int prelude_lead_tone_cycles = mTapeTiming.nomBlockTiming.firstBlockPreludeLeadToneCycles;
    float lead_tone_duration = mTapeTiming.nomBlockTiming.firstBlockLeadToneDuration; // let first block have a longer lead tone
    float other_block_lead_tone_duration = mTapeTiming.nomBlockTiming.otherBlockLeadToneDuration;// lead tone duration of all other blocks (2 s expected here but Atomulator needs 4 s)
    float last_block_gap = mTapeTiming.nomBlockTiming.lastBlockGap;
    float trailer_tone_duration = mTapeTiming.nomBlockTiming.trailerToneDuration;
 
    //
    // Write data-dependent part of UEF file
    //

    if (mVerbose) {
        cout << "Encoding data-dependent part of BBC Micro Tape File...\n";
    }

    while (file_block_iter < tapeFile.blocks.end()) {

        // Write a lead tone for the block - including a dummy byte if it is the first block
        if (mUseOriginalTiming && tapeFile.validTiming) {
            prelude_lead_tone_cycles = file_block_iter->preludeToneCycles;
            lead_tone_duration = (file_block_iter->leadToneCycles) / (2 * mBaseFrequency);
        }

        if (firstBlock) {
            double prelude_lead_tone_duration = prelude_lead_tone_cycles * (2 * mBaseFrequency);
            double postlude_lead_tone_cycles = lead_tone_duration / (2 * mBaseFrequency);
            if (!writeCarrierChunkwithDummyByte(fout, prelude_lead_tone_cycles, postlude_lead_tone_cycles)) {
                cout << "Failed to write " << prelude_lead_tone_cycles <<
                    " cycles prelude tone and dummy byte 0xaa followed by " << lead_tone_duration << " s postlude tone\n";
            }
        }
        else {
            if (!writeCarrierChunk(fout, lead_tone_duration)) {
                printf("Failed to write %f Hz lead tone of duration %f s\n", mBaseFrequency, lead_tone_duration);
            }
        }
        firstBlock = false;

        // Change lead tone duration for remaining blocks
        lead_tone_duration = other_block_lead_tone_duration;

        

        // --------------------- start of block header chunk ------------------------
        //
        // Write block header including preamble as one chunk
        //
        // block header: <preamble:1> <filename:1-10> <loadAdr:4> <execAdr:4> <blockNo:2> <blockLen:2> <blockFlag:1> <nextFileAdr:4>
        // 
        // --------------------------------------------------------------------------


        int data_len = Utility::bytes2uint(&file_block_iter->bbmHdr.blockLen[0], 4, true); // get data length

        

        // Preamble
        Bytes preamble;
        Word dummy_CRC;
        preamble.push_back(0x2a);
        if (!writeSimpleDataChunk(fout, preamble, dummy_CRC)) {
            printf("Failed to write block preamble data chunk%s\n", "");
        }

        Bytes header_data;

        // Initialise header CRC (starts from file name)
        Word header_CRC = 0;

        // Store header bytes
        if (!Utility::encodeTapeHdr(file_block_type, *file_block_iter, block_no, n_blocks, header_data)) {
            cout << "Failed to encode header for block #" << block_no << "\n";
            return false;
        }

        // Write header excluding the CRC
        if (!writeComplexDataChunk(fout, 8, 'N', -1, header_data, header_CRC)) {
            printf("Failed to write block header data chunk%s\n", "");
        }

        // Write the header CRC chunk
        Bytes header_CRC_bytes;
        dummy_CRC = 0;
        header_CRC_bytes.push_back(header_CRC / 256);
        header_CRC_bytes.push_back(header_CRC % 256);
        if (!writeSimpleDataChunk(fout, header_CRC_bytes, dummy_CRC)) {
            printf("Failed to write header CRC data chunk%s\n", "");
        }



        // --------------------------------------------------------------------------
        //
        // ---------------- End of block header chunk -------------------------------


        // --------------------- start of block data + CRC chunk ------------------------
        //
        // Write block data as one chunk
        //
        // --------------------------------------------------------------------------

        // Write block data as one chunk
        Word data_CRC = 0;
        BytesIter bi = file_block_iter->data.begin();
        Bytes block_data;
        while (bi < file_block_iter->data.end())
            block_data.push_back(*bi++);
        if (!writeComplexDataChunk(fout, 8, 'N', -1, block_data, data_CRC)) {
            printf("Failed to write block header data chunk%s\n", "");
        }

        // Write the data CRC
        Bytes data_CRC_bytes;
        data_CRC_bytes.push_back(data_CRC / 256);
        data_CRC_bytes.push_back(data_CRC % 256);
        if (!writeSimpleDataChunk(fout, data_CRC_bytes, dummy_CRC)) {
            printf("Failed to write data CRC chunk%s\n", "");
        }


        // --------------------------------------------------------------------------
        //
        // ---------------- End of block data chunk -------------------------------



        // For last block, add trailer carrier and gap
        if (block_no == n_blocks - 1) {

            // Write a trailer carrier
            if (mUseOriginalTiming && tapeFile.validTiming)
                trailer_tone_duration = (file_block_iter->trailerToneCycles) / (2 * mBaseFrequency);
            if (!writeCarrierChunk(fout, trailer_tone_duration)) {
                printf("Failed to write %f Hz lead tone (with dummy bye 0xaa) of duration %f s\n", mBaseFrequency, trailer_tone_duration);
            }

            // write a gap
            if (mUseOriginalTiming && tapeFile.validTiming)
                last_block_gap = file_block_iter->blockGap;
            if (!writeFloatPrecGapChunk(fout, last_block_gap)) {
                printf("Failed to write %f s of gap for last block\n", last_block_gap);
            }
        };
        

        file_block_iter++;


        block_no++;

    }

    return true;
}

bool UEFCodec::encodeAtom(TapeFile& tapeFile, ogzstream& fout)
{
    
    FileBlockIter file_block_iter = tapeFile.blocks.begin();

    int block_no = 0;
    int n_blocks = (int)tapeFile.blocks.size();

    // Default values for block timing (if mUseOriginalTiming is true the recored block timing will instead by used later on)
    float first_block_lead_tone_duration = mTapeTiming.nomBlockTiming.firstBlockLeadToneDuration;// lead tone duration of first block 
    float other_block_lead_tone_duration = mTapeTiming.nomBlockTiming.otherBlockLeadToneDuration;// lead tone duration of all other blocks (2 s expected here but Atomulator needs 4 s)
    float data_block_micro_lead_tone_duration = mTapeTiming.nomBlockTiming.microLeadToneDuration; //  micro lead tone (separatiing block header and block data) duration
    float block_gap = mTapeTiming.nomBlockTiming.blockGap;
    float last_block_gap = mTapeTiming.nomBlockTiming.lastBlockGap;

    float lead_tone_duration = first_block_lead_tone_duration; // let first block have a longer lead tone

    //
    // Write data-dependent part of UEF file
    //


    while (file_block_iter < tapeFile.blocks.end()) {

        // Write a lead tone for the block
        if (mUseOriginalTiming && tapeFile.validTiming)
            lead_tone_duration = (file_block_iter->leadToneCycles) / (2 * mBaseFrequency);
        if (!writeCarrierChunk(fout, lead_tone_duration)) {
            printf("Failed to write %f Hz lead tone of duration %f s\n", mBaseFrequency, lead_tone_duration);
        }
        ;

        // Change lead tone duration for remaining blocks
        lead_tone_duration = other_block_lead_tone_duration;


        // Initialise CRC

        Word CRC = 0;

        // --------------------- start of block header chunk ------------------------
        //
        // Write block header including preamble as one chunk
        //
        // block header: <flags:1> <block_no:2> <length-1:1> <exec addr:2> <load addr:2>
        // 
        // --------------------------------------------------------------------------


        int data_len = file_block_iter->atomHdr.lenHigh * 256 + file_block_iter->atomHdr.lenLow;  // get data length

        Byte b7 = (block_no < n_blocks - 1 ? 0x80 : 0x00);          // calculate flags
        Byte b6 = (data_len > 0 ? 0x40 : 0x00);
        Byte b5 = (block_no != 0 ? 0x20 : 0x00);

        Bytes header_data;

        Byte preamble[4] = { 0x2a, 0x2a, 0x2a, 0x2a };              // store preamble
        addBytes2Vector(header_data, preamble, 4);

        int name_len = 0;
        for (; name_len < sizeof(file_block_iter->atomHdr.name) && file_block_iter->atomHdr.name[name_len] != 0; name_len++);
        addBytes2Vector(header_data, (Byte*)file_block_iter->atomHdr.name, name_len); // store block name

        header_data.push_back(0xd);

        header_data.push_back(b7 | b6 | b5);                        // store flags

        header_data.push_back((block_no >> 8) & 0xff);              // store block no
        header_data.push_back(block_no & 0xff);

        header_data.push_back((data_len > 0 ? data_len - 1 : 0));   // store length - 1

        header_data.push_back(file_block_iter->atomHdr.execAdrHigh);     // store execution address
        header_data.push_back(file_block_iter->atomHdr.execAdrLow);

        header_data.push_back(file_block_iter->atomHdr.loadAdrHigh);     // store load address
        header_data.push_back(file_block_iter->atomHdr.loadAdrLow);


        if (!writeComplexDataChunk(fout, 8, 'N', -1, header_data, CRC)) {
            printf("Failed to write block header data chunk%s\n", "");
        }


        // --------------------------------------------------------------------------
        //
        // ---------------- End of block header chunk -------------------------------


        // Add micro tone between header and data
        if (mUseOriginalTiming && tapeFile.validTiming)
            data_block_micro_lead_tone_duration = file_block_iter->microToneCycles / (2 * mBaseFrequency);
        if (!writeCarrierChunk(fout, data_block_micro_lead_tone_duration)) {
            printf("Failed to write %f Hz micro lead tone of duration %f s\n", mBaseFrequency, data_block_micro_lead_tone_duration);
        }


        // --------------------- start of block data + CRC chunk ------------------------
         //
         // Write block data as one chunk
         //
         // --------------------------------------------------------------------------

        // Write block data as one chunk
        BytesIter bi = file_block_iter->data.begin();
        Bytes block_data;
        while (bi < file_block_iter->data.end())
            block_data.push_back(*bi++);
        if (!writeComplexDataChunk(fout, 8, 'N', -1, block_data, CRC)) {
            printf("Failed to write block header data chunk%s\n", "");
        }


        //
        // Write CRC
        //

        // Write CRC chunk header
        Bytes CRC_data;
        Word dummy_CRC = 0;
        CRC_data.push_back(CRC);
        if (!writeSimpleDataChunk(fout, CRC_data, dummy_CRC)) {
            printf("Failed to write CRC data chunk%s\n", "");
        }



        // --------------------------------------------------------------------------
        //
        // ---------------- End of block data chunk -------------------------------



        // Write 1 security cycle
        /*
        if (!writeSecurityCyclesChunk(fout, 1, 'W', 'P', { 1 })) {
            printf("Failed to write security bytes\n");
        }
        */

        // Write a gap at the end of the block
        if (block_no == n_blocks - 1)
            block_gap = last_block_gap;
        if (mUseOriginalTiming && tapeFile.validTiming)
            block_gap = file_block_iter->blockGap;
        if (!writeFloatPrecGapChunk(fout, block_gap)) {
            printf("Failed to write %f s and of block gap\n", block_gap);
        }

        file_block_iter++;


        block_no++;

    }

    return true;
}

bool UEFCodec::encode(TapeFile &tapeFile, string &filePath)
{
    if (tapeFile.blocks.empty()) 
        return false;

    ogzstream  fout(filePath.c_str());
    if (!fout.good()) {
        printf("Can't write to UEF file '%s'...\n", filePath.c_str());
        return false;
    }

    string tapeFileName;

    if (mVerbose)
        cout << "Encoding program '" << tapeFileName << "' as an UEF file...\n\n";


    //
    // Write data-independent part of UEF file
    //

    /*
     * Timing parameters
     *
     * Based on default or actually recorded timing
     */

    float first_block_gap = mTapeTiming.nomBlockTiming.firstBlockGap;

    int baudrate = mTapeTiming.baudRate;
    if (mUseOriginalTiming && tapeFile.validTiming)
        baudrate = tapeFile.baudRate;

    // Intialise base frequency and phase shift with values from Tape Timing
    mBaseFrequency = mTapeTiming.baseFreq;
    mPhase = mTapeTiming.phaseShift;


    // Header: UEF File!
    UefHdr hdr;
    fout.write((char*)&hdr, sizeof(hdr));


    // Add origin
    string origin = "AtomTapeUtils";
    if (!writeOriginChunk(fout, origin)) {
        printf("Failed to write origin chunk with string %s\n", origin.c_str());
    }

    // Write target
    if (!writeTargetChunk(fout)) {
        printf("Failed to write target chunk with target %s\n", _TARGET_MACHINE(mTargetMachine));
    }


    // Baudrate
    if (!writeBaudrateChunk(fout)) {
        printf("Failed to write buadrate chunk with baudrate %d.\n", baudrate);
    }


    // Phaseshift
    if (!writePhaseChunk(fout)) {
        printf("Failed to write phaseshift chunk with phase shift %d degrees\n", mPhase);
    }

    // Write base frequency
    if (!writeBaseFrequencyChunk(fout)) {
        printf("Failed to encode base frequency %f as float\n", mBaseFrequency);
    }


    // Write an intial gap before the file starts
    if (!writeFloatPrecGapChunk(fout, first_block_gap)) {
        printf("Failed to write %f s gap\n", first_block_gap);
    }

    if (mTargetMachine <= BBC_MASTER) {
        encodeBBM(tapeFile, fout);
        tapeFileName = tapeFile.blocks[0].atomHdr.name;
    }
    else if (mTargetMachine <= ACORN_ATOM) {
        encodeAtom(tapeFile, fout);
        tapeFileName = tapeFile.blocks[0].atomHdr.name;
    }
    else {
        cout << "Unknown target machine " << hex << mTargetMachine << "\n";
        return false;
    }

    fout.close();

    if (!fout.good()) {
        printf("Failed writing to file %s for compression output\n", filePath.c_str());
        return false;
    }

    if (mVerbose)
        cout << "\nDone encoding program '" << tapeFileName << "' as an UEF file...\n\n";

    return true;

}

/*
     * Decode UEF file but only print its content rather than converting a Tape File
     */
bool UEFCodec::inspect(string& uefFileName)
{
    TapeFile tape_file(TargetMachine::UNKNOWN_TARGET);

    mInspect = true;

    return decode(uefFileName, tape_file);
}

void UEFCodec::resetCurrentBlockInfo()
{
    mCurrentBlockInfo.block_gap = -1;
    mCurrentBlockInfo.prelude_lead_tone_cycles = -1;
    mCurrentBlockInfo.lead_tone_cycles = -1;
    mCurrentBlockInfo.micro_tone_cycles = -1;
    mCurrentBlockInfo.trailer_tone_cycles = -1;
    mCurrentBlockInfo.start = -1;
    mCurrentBlockInfo.end = -1;
    mCurrentBlockInfo.phase_shift = -1;
}

void UEFCodec::pullCurrentBlock()
{
    mCurrentBlockInfo = mBlockInfo.back();
    mBlockInfo.pop_back();
}

void UEFCodec::pushCurrentBlock()
{
    mBlockInfo.push_back(mCurrentBlockInfo);

    resetCurrentBlockInfo();
}

bool UEFCodec::getUEFdata(string& uefFileName, Bytes &data)
{
    igzstream fin(uefFileName.c_str());

    mBlockInfo.clear();

    resetCurrentBlockInfo();

    if (!fin.good()) {

        printf("Failed to open file '%s'\n", uefFileName.c_str());
        return false;
    }

    if (mVerbose)
        cout << "Decoding UEF file '" << uefFileName << "'...\n\n";

    filesystem::path fin_p = uefFileName;
    string file_name = fin_p.stem().string();

    if (mTargetMachine != TargetMachine::UNKNOWN_TARGET) {

        if (mVerbose)
            cout << "Initial block reading state is " << _BLOCK_STATE(mBlockState) << "...\n";

    }



    UefHdr hdr;

    fin.read((char*)&hdr, sizeof(hdr));

    if (strcmp(hdr.uefTag, "UEF File!") != 0) {
        printf("File '%s' has no valid UEF header\n", uefFileName.c_str());
        fin.close();
        return false;
    }
    if (mVerbose)
        cout << "UEF Header with major version " << (int)hdr.uefVer[1] << " and minor version " << (int)hdr.uefVer[0] << " read.\n";

    if (mVerbose)
        cout << "Initial defaults values are " << mBaudRate << " Baud, " << mPhase <<
        " degrees and a base frequency of " << mBaseFrequency << " Hz\n";

    ChunkHdr chunk_hdr;

    while (fin.read((char*)&chunk_hdr, sizeof(chunk_hdr))) {

        int chunk_id = chunk_hdr.chunkId[0] + chunk_hdr.chunkId[1] * 256;
        long chunk_sz = chunk_hdr.chunkSz[0] + (chunk_hdr.chunkSz[1] << 8) + (chunk_hdr.chunkSz[2] << 16) + (chunk_hdr.chunkSz[3] << 24);

        if (mVerbose && !mInspect)
            cout << "\nChunk " << _CHUNK_ID(chunk_id) << " of size " << dec << chunk_sz << " read at state " << _BLOCK_STATE(mBlockState) << "...\n";

        switch (chunk_id) {

        case TARGET_CHUNK: // Target machine
        {
            if (chunk_sz != 1) {
                printf("Size of target machine chunk 0005 has an incorrect chunk size %ld (should have been %d)\n", chunk_sz, 1);
                return false;
            }
            TargetMachineChunk chunk;
            fin.read((char*)&chunk.targetMachine, sizeof(chunk.targetMachine));
            if (_TARGET_MACHINE(chunk.targetMachine) == "???") {
                cout << "Invalid target machine " << (int)chunk.targetMachine << "\n";
            }
            mTargetMachine = (TargetMachine)chunk.targetMachine;
            if (mVerbose)
                cout << "Target machine chunk 0005 of size " << chunk_sz << " and specifying target " << _TARGET_MACHINE(mTargetMachine) << ".\n";
            break;
        }

        case FLOAT_GAP_CHUNK: // Floating-point gap
        {
            if (chunk_sz != 4) {
                printf("Size of Format change chunk 0116 has an incorrect chunk size %ld (should have been %d)\n", chunk_sz, 4);
                return false;
            }
            FPGapChunk gap_chunk;
            fin.read((char*)gap_chunk.gap, sizeof(gap_chunk.gap));
            float gap;

            if (!decodeFloat(gap_chunk.gap, gap)) {
                printf("Failed to decode IEEE 754 gap%s\n", "");
            }
            if (mVerbose)
                cout << "Format change chunk 0116 of size " << chunk_sz << " and specifying a gap of " << gap << " s.\n";
            if (!updateBlockState(CHUNK_TYPE::GAP_CHUNK, gap, 0)) {
                cout << "Unexpected Format change chunk 0116!\n";
                return false;
            }
            break;
        }

        case BASE_FREQ_CHUNK: // Base frequency
        {
            if (chunk_sz != 4) {
                printf("Size of Format change chunk 0113 has an incorrect chunk size %ld (should have been %d)\n", chunk_sz, 4);
                return false;
            }
            BaseFreqChunk chunk;
            fin.read((char*)chunk.frequency, sizeof(chunk.frequency));
            if (!decodeFloat(chunk.frequency, mBaseFrequency)) {
                printf("Failed to decode IEEE 754 gap%s\n");
            }
            if (mVerbose)
                cout << "Format change chunk 0113 of size " << chunk_sz << " and specifying a frequency of " << mBaseFrequency << ".Hz\n";
            break;
        }


        case BAUD_RATE_CHUNK: // Format Change Chunk 0117: baudrate = value
        {
            if (chunk_sz != 2) {
                printf("Size of Format change chunk 0117 has an incorrect chunk size %ld (should have been %d)\n", chunk_sz, 2);
                return false;
            }
            BaudRateChunk baudrate_chunk;
            fin.read((char*)baudrate_chunk.baudRate, sizeof(baudrate_chunk.baudRate));
            mBaudRate = (baudrate_chunk.baudRate[0] + (baudrate_chunk.baudRate[1] << 8));
            if (mVerbose)
                cout << "Format change chunk 0117 of size " << chunk_sz << " and baudrate of " << mBaudRate << ".\n";

            break;
        }
        case PHASE_CHUNK: // Phase Shift Change Chunk 0115: phase shift = value
        {
            if (chunk_sz != 2) {
                printf("Size of Phase Shift change chunk 0115 has an incorrect chunk size %ld (should have been %d)\n", chunk_sz, 2);
                return false;
            }
            PhaseChunk phase_chunk;
            phase_chunk.chunkHdr = chunk_hdr;
            fin.read((char*)phase_chunk.phase_shift, sizeof(phase_chunk.phase_shift));
            mPhase = (phase_chunk.phase_shift[0] + (phase_chunk.phase_shift[1] << 8));
            if (mVerbose)
                cout << "Phase change chunk 0115 of size " << chunk_sz << " and specifying a 1/2 cycle of " << mPhase << " degrees.\n";

            mCurrentBlockInfo.phase_shift = mPhase;
            break;
        }

        case BAUDWISE_GAP_CHUNK: // Baudwise Gap Chunk 0112: gap = value / (baud_rate  * 2)
        {
            if (chunk_sz != 2) {
                printf("Size of Baudwise gap chunk 0112 has an incorrect chunk size %ld (should have been %d)\n", chunk_sz, 2);
                return false;
            }
            BaudwiseGapChunk gap_chunk;
            gap_chunk.chunkHdr = chunk_hdr;
            fin.read((char*)gap_chunk.gap, sizeof(gap_chunk.gap));
            double gap_duration = double(gap_chunk.gap[0] + (gap_chunk.gap[1] << 8)) / (mBaudRate * 2);
            if (mVerbose)
                cout << "Baudwise gap chunk 0112 of size " << chunk_sz << " and specifying a duration of " << gap_duration << " s.\n";
            if (!updateBlockState(CHUNK_TYPE::GAP_CHUNK, gap_duration, 0)) {
                cout << "Unexpected Format change chunk 0116!\n";
                return false;
            }
            break;
        }

        case CARRIER_TONE_CHUNK: // Carrier Chunk 0110: duration = value / (mBaseFrequency * 2)
        {
            if (chunk_sz != 2) {
                printf("Size of Carrier chunk 0110 has an incorrect chunk size %ld (should have been %d)\n", chunk_sz, 2);
                return false;
            }
            CarrierChunk tone_chunk;
            tone_chunk.chunkHdr = chunk_hdr;
            fin.read((char*)tone_chunk.duration, sizeof(tone_chunk.duration));
            double tone_duration = double(tone_chunk.duration[0] + (tone_chunk.duration[1] << 8)) / (mBaseFrequency * 2);
            if (mVerbose)
                cout << "High tone chunk 0110 of size " << chunk_sz << " and specifying a duration of " << tone_duration << " s.\n";
            if (!updateBlockState(CHUNK_TYPE::CARRIER_CHUNK, tone_duration, 0)) {
                cout << "Unexpected high tone chunk!\n";
                return false;
            }
            break;
        }

        case CARRIER_TONE_WITH_DUMMY_BYTE: // Carrier Chunk with dummy byte 0111: durations in carrier tone cycles
        {
            if (chunk_sz != 4) {
                printf("Size of Carrier chunk 0111 has an incorrect chunk size %ld (should have been %d)\n", chunk_sz, 2);
                return false;
            }
            CarrierChunkDummy tone_chunk;
            tone_chunk.chunkHdr = chunk_hdr;
            fin.read((char*)&tone_chunk.duration[0], sizeof(tone_chunk.duration));
            int first_cycles = Utility::bytes2uint((Byte*)&tone_chunk.duration[0], 2, true);
            float first_duration = first_cycles / (2 * mBaseFrequency);
            int following_cycles = Utility::bytes2uint((Byte*)&tone_chunk.duration[2], 2, true);
            float following_duration = following_cycles / (2 * mBaseFrequency);

            if (mVerbose)
                cout << "Carrier chunk with dummy byte 0111 of size " << chunk_sz << " and specifying " << first_cycles <<
                " cycles of carrier followed by a dummy byte 0xaa, and ending with " << following_duration << "s of carrier.\n";
            if (!updateBlockState(CHUNK_TYPE::CARRIER_DUMMY_CHUNK, following_duration, first_cycles)) {
                cout << "Unexpected high tone chunk!\n";
                return false;
            }

            data.push_back(0xaa);
            break;
        }


        case SECURITY_CHUNK: // Security cycles
        {
            SecurityCyclesChunkHdr hdr;
            hdr.chunkHdr = chunk_hdr;

            // Get #cycles
            fin.read((char*)&hdr.nCycles, sizeof(hdr.nCycles));
            int n_cycles = hdr.nCycles[0] + (hdr.nCycles[1] << 8);

            // Get first & last pulse info
            fin.read((char*)&hdr.firstPulse, sizeof(hdr.firstPulse));
            fin.read((char*)&hdr.lastPulse, sizeof(hdr.lastPulse));

            // Get cycle bytes        
            int n_cycle_bytes = ceil((float)n_cycles / 8);
            Bytes cycles;
            string cycle_bytes = "[";
            for (int i = 0; i < n_cycle_bytes; i++) {
                Byte b;
                fin.read((char*)&b, 1);
                if (i < n_cycle_bytes - 1)
                    cycle_bytes += to_string(b) + ", ";
                else
                    cycle_bytes += to_string(b) + "]";
                cycles.push_back(b);
            }
            string cycle_string = decode_security_cycles(hdr, cycles);

            if (mVerbose) {
                cout << "Security cycles chunk 0114 of size " << chunk_sz << " specifying " << n_cycles;
                cout << " with format " << hdr.firstPulse << hdr.lastPulse << " and cycle bytes " << cycle_bytes << ".\n";
            }

            break;
        }

        case ORIGIN_CHUNK: // Origin Chunk 0000
        {
            string origin = "";
            for (int pos = 0; pos < chunk_sz; pos++) {
                char c;
                fin.get(c);
                origin += c;
            }
            if (mVerbose)
                cout << "Origin chunk 000 of size " << chunk_sz << " and text '" << origin << "'.\n";
            break;
        }

        case SIMPLE_DATA_CHUNK: // Data Block Chunk 0100
        {
            if (mVerbose)
                cout << "Data block chunk 0100 of size " << chunk_sz << ".\n";
            int count = 0;
            for (int i = 0; i < chunk_sz; i++) {
                Byte byte;
                fin.read((char*)&byte, 1);
                data.push_back(byte);
            }
            if (!updateBlockState(CHUNK_TYPE::DATA_CHUNK, chunk_sz * 10 * 4 / mBaseFrequency, 0)) {
                cout << "Unepxected Data block chunk 0100\n";
                return false;
            }
            break;
        }

        case COMPLEX_DATA_CHUNK: // Data Block Chunk 0104
        {
            ComplexDataBlockChunkHdr hdr;
            hdr.chunkHdr = chunk_hdr;
            fin.read((char*)&hdr.bitsPerPacket, sizeof(hdr.bitsPerPacket));
            fin.read((char*)&hdr.parity, sizeof(hdr.parity));
            fin.read((char*)&hdr.stopBitInfo, sizeof(hdr.stopBitInfo));
            int nStopBits = (hdr.stopBitInfo >= 0x80 ? hdr.stopBitInfo - 256 : hdr.stopBitInfo);

            if (mVerbose) {
                cout << "Data block chunk 0104 of size " << chunk_sz << " and packet format ";
                cout << (int)hdr.bitsPerPacket << (char)hdr.parity << (int)nStopBits << "\n";
            }
            for (int i = 0; i < chunk_sz - 3; i++) {
                Byte byte;
                fin.read((char*)&byte, 1);
                data.push_back(byte);
            }
            if (!updateBlockState(CHUNK_TYPE::DATA_CHUNK, chunk_sz * 10 * 4 / mBaseFrequency, 0)) {
                cout << "Unepxected Data block chunk 0100\n";
                return false;
            }
            break;
        }

        default: // Unknown chunk
        {
            printf("Unknown chunk %.4x of size %ld.\n", chunk_id, chunk_sz);
            fin.ignore(chunk_sz);
            break;
        }

        }

    }

    // If the final block is not followed by a gap, then
    // the block needs still to have a gap (0) and  it
    // need to be added to the list of blocks properly.
    if (!mInspect && mBlockState != BLOCK_STATE::READING_GAP) {
        if (mTargetMachine <= BBC_MASTER) {
            int trailer_tone_cycles = mCurrentBlockInfo.lead_tone_cycles;
            pullCurrentBlock();
            mCurrentBlockInfo.trailer_tone_cycles = trailer_tone_cycles;
            mCurrentBlockInfo.block_gap = 0;
            pushCurrentBlock();
        }
        else {
            mCurrentBlockInfo.block_gap = 0;
            pushCurrentBlock();
        }
    }

    fin.close();

    if (!fin.eof()) {
        printf("Failed while reading compressed input file %s\n", uefFileName.c_str());
        return false;
    }

    if (mVerbose) {
        if (!mInspect)
            cout << "\n\nUEF File bytes read - now to decode them as an Acorn Atom/BBC Micro Tape File\n\n";
        else
            cout << "\n\nUEF File bytes read - now all the bytes will be output in on go!\n\n";
    }
}

bool UEFCodec::decodeFile(string uefFilename, Bytes &data, TapeFile& tapeFile, BytesIter& data_iter)
{
    // Clear Tape File
    if (!mInspect) {
        tapeFile.blocks.clear();
        tapeFile.complete = true;
        tapeFile.isBasicProgram = false;
        tapeFile.validTiming = false;
    }

    bool success;
    if (mTargetMachine <= BBC_MASTER) {     
        success = decodeBBMFile(data, tapeFile, data_iter);
        if (success)
            tapeFile.validFileName = tapeFile.blocks[0].blockName();
    } 
    else if (!mInspect) {
        success = decodeAtomFile(data, tapeFile, data_iter);
        if (success)
            tapeFile.validFileName = tapeFile.blocks[0].blockName();
    }
    else
        success = inspectFile(data);

    // Remove timing information (in mBlockInfo) saved for the decoded file
    // in case another file would follow. This will ensure
    // that the timing information saved reflect the next file.
    if (!mInspect && tapeFile.blocks.size() > 0) {
        int n = tapeFile.blocks.size();
        if (mBlockInfo.size() < n)
            n = mBlockInfo.size();
        mBlockInfo.erase(mBlockInfo.begin(), mBlockInfo.begin() + n);
    }

    return success;
}

bool UEFCodec::decode(string& uefFileName, TapeFile& tapeFile)
{
    Bytes data;
    if (!getUEFdata(uefFileName, data)) {
        cout << "Failed to parse UEF file '" << uefFileName << "'!\n";
        return false;
    }

    BytesIter data_iter = data.begin();


    //
    // Iterate over the collected data and create an Tape File with blocks from it
    //
    bool success = decodeFile(uefFileName, data, tapeFile, data_iter);

    if (mVerbose)
        cout << "\nDone decoding UEF file '" << uefFileName << "'...\n\n";

    return success;
}

/*
 * Parse tape byte stream and create a BBC Micro Tape Structure from it.
 *.
 * Timing and format of a BBC Micro File represented as UEF chunks are:
 * <4 cycles of lead carrier prelude > <dummy byte 0xaa> <5.1s lead carrier postlude> <header with CRC>
 *  {<data with CRC> <0.9s lead carrier> <data with CRC } <5.3s trailer carrier> <1.8s gap>
 * 
 * The dummy byte could be part of a 0x0111 'carrier chunk with dummy byte' or
 * being explictly inserted as a 0x0100 data chunk (of size 1) between two 0x0110 carrier chunks
 * 
 */
bool UEFCodec::decodeBBMFile(Bytes &data, TapeFile &tapeFile, BytesIter &data_iter)
{
    TargetMachine file_block_type = BBC_MODEL_B;
    int block_no = 0;
    bool is_last_block = false;
    bool is_first_block = true;
    string tape_file_name;
    tapeFile.fileType = BBC_MODEL_B;
    tapeFile.blocks.clear();
    uint32_t adr_offset = 0;

    while (data_iter < data.end() && !is_last_block) {

        if (is_last_block)
            is_first_block = true;

        //
        // Read one block
        // 

        FileBlock block(BBC_MODEL_B);
        Word hdr_CRC;
        BytesIter hdr_start = data_iter;

        // Skip dummy byte if it is the first block
        if (is_first_block) {
            Word dummy_CRC;
            Byte dummy_byte;
            if (!read_bytes(data_iter, data, 1, dummy_CRC, &dummy_byte)) {
                printf("Failed to read dummy byte for block #%d.\n", block_no);
                return false;
            }
            is_first_block = false;
        }

        // Get timing data related to the block (recorded previously during reading of the UEF chunks)
        if (block_no >= mBlockInfo.size() || !Utility::initTapeHdr(block, mBlockInfo[block_no])) {
            cout << "Failed to initialise BBC Micro Tape Header for block #" << block_no << "\n";
            return false;
        }
        
        // Read preamble (1 x 0x2a)
        if (!check_bytes(data_iter, data, 1, hdr_CRC, 0x2a)) {
            printf("Failed to read preamble bytes for block #%d.\n", block_no);
            //return false;
        }

        hdr_CRC = 0;

        // Read block name (up to 10 chars terminated by 0x0)
        
        if (!Utility::readTapeFileName(mTargetMachine, data_iter, data, hdr_CRC, &block.bbmHdr.name[0])) {
            printf("Failed to read name bytes for block #%d.\n", block_no);
            return false;
        }


        // Read remaining of block header
        BBMTapeBlockHdr hdr;
        if (!read_bytes(data_iter, data, sizeof(hdr), hdr_CRC, (Byte*)&hdr)) {
            printf("Failed to read last part of header for block #%d '%s'.\n", block_no, block.bbmHdr.name);
            return false;
        }

        // Fill the Tape File Block with it
        // Extract header information and update BBM block with it
        uint32_t exec_adr, next_adr, load_adr, data_len;
        if (!Utility::decodeBBMTapeHdr(hdr, block, load_adr, exec_adr, data_len, block_no, next_adr, is_last_block)) {
            cout << "Failed to decode BBC Micro Tape Header!\n";
            return false;
        }

        if (mVerbose)
            Utility::logTAPBlockHdr(block, adr_offset);
 


        if (DEBUG_LEVEL == DBG)
            Utility::logData(0x0000, hdr_start, data_iter - hdr_start - 1);

        // Read header CRC
        Word read_hdr_CRC;
        if (!read_word(data_iter, data, read_hdr_CRC, false)) {
            printf("Failed to read header CRC for block #%d '%s'.\n", block_no, block.bbmHdr.name);
            return false;
        }


        // Check header CRC
        if (read_hdr_CRC != hdr_CRC) {
            printf("Incorrect header CRC 0x%.4x (expected 0x%.4x) for block #%d '%s'.\n", read_hdr_CRC, hdr_CRC, block_no, block.bbmHdr.name);
            return false;
        }

        Word data_CRC = 0;

        // Read block data
        if (!read_bytes(data_iter, data, data_len, data_CRC, block.data)) {
            printf("Failed to read data part of block #%d '%s'.\n", block_no, block.bbmHdr.name);
            return false;
        }
        BytesIter bi = block.data.begin();
        if (DEBUG_LEVEL == DBG)
            Utility::logData(load_adr, bi, block.data.size());


        // Read data CRC
        Word read_data_CRC;
        if (!read_word(data_iter, data, read_data_CRC, false)) {
            printf("Failed to read data block CRC for block #%d '%s'.\n", block_no, block.bbmHdr.name);
            return false;
        }
 

        // Check data CRC
        if (read_data_CRC != data_CRC) {
            printf("Incorrect data CRC 0x%.4x (expected 0x%.4x) for block #%d '%s'.\n", read_data_CRC, data_CRC, block_no, block.bbmHdr.name);
            return false;
        }

        adr_offset += block.data.size();

        tapeFile.blocks.push_back(block);

        if (block_no == 0)
            tape_file_name = block.bbmHdr.name; 
        else if (tape_file_name != block.bbmHdr.name) {
            printf("Name of block #%d '%s' not consistent with tape file name '%s'.\n", block_no, block.bbmHdr.name, tape_file_name.c_str());
            return false;
        }

        block_no++; // predict next block no

        
    }

    if (!is_last_block) {
        printf("Missing last block for BBC Micro File '%s'.\n", tape_file_name.c_str());
        return false;
    }

    return true;
}

bool UEFCodec::decodeAtomFile(Bytes &data, TapeFile& tapeFile, BytesIter& data_iter)
{
    BlockType block_type = BlockType::Unknown;
    string tape_file_name;
    int block_no = 0;
    tapeFile.fileType = ACORN_ATOM;
    tapeFile.blocks.clear();

    while (data_iter < data.end() && !(block_type & BlockType::Last)) {

        //
        // Read one block
        // 

        FileBlock block(ACORN_ATOM);
        Word CRC = 0;
        BytesIter hdr_start = data_iter;

        if (block_no >= mBlockInfo.size() || !Utility::initTapeHdr(block, mBlockInfo[block_no])) {
            cout << "Failed to initialise Atom Tape Header for block #" << block_no << "\n";
            return false;
        }

        // Read preamble (4 x 0x2a)
        if (!check_bytes(data_iter, data, 4, CRC, 0x2a)) {
            printf("Failed to read preamble bytes for block #%d.\n", block_no);
            return false;
        }


        // Read block name (up to 13 chars terminated by 0xd)
        if (!Utility::readTapeFileName(mTargetMachine, data_iter, data, CRC, &block.atomHdr.name[0])) {
            printf("Failed to read name bytes for block #%d.\n", block_no);
            return false;
        }


        // Read remaining of block header
        AtomTapeBlockHdr hdr;
        if (!read_bytes(data_iter, data, sizeof(hdr), CRC, (Byte*)&hdr)) {
            printf("Failed to read last part of header  for block #%d '%s'.\n", block_no, block.atomHdr.name);
            return false;
        }

        int load_address, exec_adr, data_len;
        if (!Utility::decodeAtomTapeHdr(hdr, block, load_address, exec_adr, data_len, block_no, block_type))
            return false;
        

        if (mVerbose)
            Utility::logAtomTapeHdr(hdr, block.atomHdr.name);
 
        if (DEBUG_LEVEL == DBG)
            Utility::logData(0x0000, hdr_start, data_iter - hdr_start - 1);

        // Read block data
        if (!read_bytes(data_iter, data, data_len, CRC, block.data)) {
            printf("Failed to read data part of block #%d '%s'.\n", block_no, block.atomHdr.name);
            return false;
        }
        BytesIter bi = block.data.begin();
        if (DEBUG_LEVEL == DBG)
            Utility::logData(load_address, bi, block.data.size());


        // Read CRC
        if (!(data_iter < data.end())) {
            printf("Failed to read CRC for block #%d '%s'.\n", block_no, block.atomHdr.name);
            return false;
        }
        Word read_CRC = *data_iter++;

        // Check CRC
        if (read_CRC != CRC) {
            printf("Incorrect CRC 0x%.2x (expected 0x%.2x) for block #%d '%s'.\n", read_CRC, CRC, block_no, block.atomHdr.name);
            return false;
        }

        tapeFile.blocks.push_back(block);

        if (block_type & BlockType::First)
            tape_file_name = block.atomHdr.name;
        else if (tape_file_name != block.atomHdr.name) {
            printf("Name of block #%d '%s' not consistent with tape file name '%s'.\n", block_no, block.atomHdr.name, tape_file_name.c_str());
            return false;
        }

        block_no++; // predict next block no

    }

    if (!(block_type & BlockType::Last)) {
        printf("Missing last block for Atom File '%s'.\n", tape_file_name.c_str());
        return false;
    }

    return true;

}

bool UEFCodec::inspectFile(Bytes data)
{
    BytesIter data_iter = data.begin();

    Utility::logData(0x0, data_iter, data.size());

    return true;

}


bool UEFCodec::read_word(BytesIter& data_iter, Bytes& data, Word& word, bool little_endian)
{
    Word dummy_CRC;
    Bytes bytes;
    if (!read_bytes(data_iter, data, 2, dummy_CRC, bytes)) {
        printf("Failed to read word.\n");
        return false;
    }

    if (little_endian)
        word = bytes[1] * 256 + bytes[0];
    else
        word = bytes[0] * 256 + bytes[1];

    return true;
}
bool UEFCodec::read_bytes(BytesIter& data_iter, Bytes& data, int n, Word & CRC, Bytes &block_data)
{
    int i = 0;

    for (i = 0; i < n && data_iter < data.end(); data_iter++) {
        block_data.push_back(*data_iter);
        Utility::updateCRC(mTargetMachine, CRC, *data_iter);
        i++;
    }

    if (i < n) {
        printf("Only %d of expected %d UEF bytes were read.\n", i, n);
        return false;
    }

    return true;
}

bool UEFCodec::read_bytes(BytesIter& data_iter, Bytes& data, int n, Word& CRC, Byte bytes[])
{
    int i = 0;

    for (i = 0; i < n && data_iter < data.end(); data_iter++) {
        bytes[i] = *data_iter;
        Utility::updateCRC(mTargetMachine, CRC, *data_iter);
        i++;
    }

    if (i < n) {
        printf("Only %d of expected %d UEF bytes were read.\n", i, n);
        return false;
    }

    return true;
}

bool UEFCodec::check_bytes(BytesIter& data_iter, Bytes& data, int n, Word& CRC, Byte refVal)
{
    int i = 0;

    for (i = 0; i < n && data_iter < data.end(); data_iter++) {
        if (*data_iter != refVal) {
            printf("UEF Byte %.2x was %x when expecting %2x.\n", i, *data_iter, refVal);
            return false;
        }
        Utility::updateCRC(mTargetMachine, CRC, *data_iter);
        i++;
    }

    if (i < n) {
        printf("Only %d of expected %d UEF bytes were read.\n", i, n);
        return false;
    }

    return true;
}


bool UEFCodec::writeUEFHeader(ogzstream&fout, Byte majorVersion, Byte minorVersion)
{
    // Header: UEF File!
    UefHdr hdr;
    fout.write((char*)&hdr, sizeof(hdr));

    return true;
}

 bool UEFCodec::writeBaseFrequencyChunk(ogzstream&fout)
{

     BaseFreqChunk chunk;
     if (!encodeFloat(mBaseFrequency, chunk.frequency)) {
         printf("Failed to encode base frequency %f as float.\n", mBaseFrequency);
         return false;
     }
     fout.write((char*) &chunk, sizeof(chunk));

     if (mVerbose)
         printf("Base frequency chunk 0113 specifying %.0f Hz written.\n", mBaseFrequency);

     return true;
}

 bool UEFCodec::writeTargetChunk(ogzstream& fout)
 {

     TargetMachineChunk chunk;
     chunk.targetMachine = mTargetMachine;
     fout.write((char*)&chunk, sizeof(chunk));

     if (mVerbose)
         printf("Target machine chunk 0005 specifying a target %s written.\n", _TARGET_MACHINE(mTargetMachine));

     return true;
 }

 // Write a float-precision gap 
bool UEFCodec::writeFloatPrecGapChunk(ogzstream&fout, float duration)
{
    
    FPGapChunk chunk;

    if (!encodeFloat(duration, chunk.gap)) {
        printf("Failed to encode gap as float of duriaion %f.\n", (double) duration);
        return false;
    }
    fout.write((char*)&chunk, sizeof(chunk));

    if (mVerbose)
        printf("Gap 0116 chunk specifying a gap of %.2f s written.\n", duration);

    return true;
}

// Write an integer-precision gap
bool UEFCodec::writeIntPrecGapChunk(ogzstream& fout, float duration)
{
    BaudwiseGapChunk chunk;
    int gap = (int) round(duration * mBaudRate * 2);
    chunk.gap[0] = gap & 0xff;
    chunk.gap[1] = (gap >> 8) & 0xff;
    fout.write((char*) &chunk, sizeof(chunk));

    if (mVerbose)
        printf("Gap 0112 chunk specifying a gap of %.2f s written.\n", duration);

    return true;
}

// Write security cycles
bool UEFCodec::writeSecurityCyclesChunk(ogzstream& fout, int nCycles, Byte firstPulse, Byte lastPulse, Bytes cycles)
{
    SecurityCyclesChunkHdr hdr;


    // Set up header
    hdr.nCycles[0] = nCycles & 0xff;
    hdr.nCycles[1] = (nCycles >> 8) & 0xff;
    hdr.nCycles[2] = (nCycles >> 16) & 0xff;;
    hdr.firstPulse = firstPulse;
    hdr.lastPulse = lastPulse;
    int chunk_sz = cycles.size() + 5;
    hdr.chunkHdr.chunkSz[0] = chunk_sz & 0xff;
    hdr.chunkHdr.chunkSz[1] = (chunk_sz >> 8) & 0xff;
    hdr.chunkHdr.chunkSz[2] = (chunk_sz >> 16) & 0xff;

    // Write header
    fout.write((char*)&hdr, sizeof(hdr));

    // Write cycle bytes
    string cycle_bytes = "[";
    for (int i = 0; i < cycles.size(); i++) {
        fout.write((char*)&cycles[i], 1);
        if (i < cycles.size() - 1)
            cycle_bytes += to_string(cycles[i]) + ", ";
        else
            cycle_bytes += to_string(cycles[i]) + "]";
    }

    // Decode cycles (for debugging message only)
    string cycle_string = decode_security_cycles(hdr, cycles);

    if (mVerbose)
        printf("Security cycles chunk 0114 of size %ld specifying %d security cycles with format '%c%c' and cycle bytes '%s' %s written\n",
        (long int) chunk_sz, nCycles, hdr.firstPulse, hdr.lastPulse, cycle_string.c_str(), cycle_bytes.c_str()
        );

    return true;
}

// Decode cycles as string where high pulse = '1',
// low pulse = '0', high frequency = 'H' and
// low frequency = 'L'
string UEFCodec::decode_security_cycles(SecurityCyclesChunkHdr &hdr, Bytes cycles) {

    string cycle_string;
    int n_cycles = hdr.nCycles[0] + ((hdr.nCycles[1] << 8) & 0xff00) + ((hdr.nCycles[2] << 16) & 0xff0000);
    if (hdr.firstPulse == 'P') {
        cycle_string += "1";
        n_cycles--;
    }
    else if (hdr.lastPulse == 'P')
        n_cycles--;
    int f = 0x80;
    BytesIter bytes_iter = cycles.begin();
    for (int i = 0; i < n_cycles; i++) {
        if (i > 0 && i % 8 == 0) {
            bytes_iter++;
            f = 0x80;
        }
        int b = *bytes_iter;
        if ((b & f) == f)
            cycle_string += "H";
        else
            cycle_string += "L";
        f = f >> 2;
    }
    if (hdr.lastPulse == 'P')
        cycle_string += "0";

    return cycle_string;
}

// write baudrate 
bool UEFCodec::writeBaudrateChunk(ogzstream&fout)
{
    BaudRateChunk chunk;
    chunk.baudRate[0] = mBaudRate & 0xff;
    chunk.baudRate[1] = mBaudRate >> 8;
    fout.write((char*)&chunk, sizeof(chunk));

    if (mVerbose)
        printf("Baudrate chunk 0117 specifying a baud rate of %d written.\n", mBaudRate);

    return true;
}

// Write phase_shift chunk
bool UEFCodec::writePhaseChunk(ogzstream &fout)
{
    
    PhaseChunk chunk;
    chunk.phase_shift[0] = mPhase & 0xff;
    chunk.phase_shift[1] = mPhase >> 8;
    fout.write((char*)&chunk, sizeof(chunk));

    if (mVerbose)
        printf("Phase chunk 0115 specifying a phase_shift of %d degrees written.\n", mPhase);
    return true;
}

bool UEFCodec::writeCarrierChunk(ogzstream &fout, float duration)
{
    // Write a high tone
    CarrierChunk chunk;
    int cycles = (int) round(duration * mBaseFrequency * 2);
    chunk.duration[0] = cycles % 256;
    chunk.duration[1] = cycles / 256;
    fout.write((char*)&chunk, sizeof(chunk));

    if (mVerbose)
        printf("Carrier chunk 0110 specifying a %.2f s tone written.\n", duration);

    return true;
}
bool UEFCodec::writeCarrierChunkwithDummyByte(ogzstream& fout, int firstCycles, int followingCycles)
{
    // Write a high tone
    CarrierChunkDummy chunk;
    float duration = followingCycles / (2 * mBaseFrequency);
    chunk.duration[0] = firstCycles % 256;
    chunk.duration[1] = firstCycles / 256;
    chunk.duration[2] = followingCycles % 256;
    chunk.duration[3] = followingCycles / 256;
    fout.write((char*)&chunk, sizeof(chunk));

    if (mVerbose)
        printf("Carrier chunk 0111 specifying %d cycles of a carrier followed by a dummy byte 0xaa and ending with %.2f s of carrier written.\n", firstCycles, duration);

    return true;
}

bool UEFCodec::writeComplexDataChunk(ogzstream &fout, Byte bitsPerPacket, Byte parity, Byte stopBitInfo, Bytes data, Word & CRC)
{

    ComplexDataBlockChunkHdr chunk;
    int block_size = data.size() + 3; // add 3 bytes for the #bits/packet, parity & stop bit
    chunk.chunkHdr.chunkSz[0] = block_size & 0xff;
    chunk.chunkHdr.chunkSz[1] = (block_size >> 8) & 0xff;
    chunk.chunkHdr.chunkSz[2] = (block_size >> 16) & 0xff;
    chunk.chunkHdr.chunkSz[3] = (block_size >> 23) & 0xff;
    chunk.bitsPerPacket = bitsPerPacket;
    chunk.parity = parity;
    chunk.stopBitInfo = stopBitInfo;
    fout.write((char*)&chunk, sizeof(chunk));

    BytesIter data_iter = data.begin();
    for (int i = 0; i < block_size - 3; i++) {
        Byte b = *data_iter++;
        Utility::updateCRC(mTargetMachine, CRC, b);
        fout.write((char*)&b, sizeof(b));
    }

    if (mVerbose)
        printf("Data chunk 0104 of format %d%c%d and size %d written.\n", bitsPerPacket, parity, (stopBitInfo>=128?stopBitInfo-256: stopBitInfo), block_size);
    return true;
}

bool UEFCodec::writeSimpleDataChunk(ogzstream &fout, Bytes data, Word &CRC)
{
    SimpleDataBlockChunkHdr chunk;
    int block_size = data.size();
    chunk.chunkHdr.chunkSz[0] = block_size & 0xff;
    chunk.chunkHdr.chunkSz[1] = (block_size >> 8) & 0xff;
    chunk.chunkHdr.chunkSz[2] = (block_size >> 16) & 0xff;
    chunk.chunkHdr.chunkSz[3] = (block_size >> 23) & 0xff;
    fout.write((char*)&chunk, sizeof(chunk));

    BytesIter data_iter = data.begin();
    for (int i = 0; i < block_size; i++) {
        Byte b = *data_iter++;
        Utility::updateCRC(mTargetMachine, CRC, b);
        fout.write((char*)&b, sizeof(b));
    }

    if (mVerbose)
        printf("Data chunk 0100 of size %d written.\n", block_size);

    return true;
}

// Write origin chunk
bool UEFCodec::writeOriginChunk(ogzstream &fout, string origin)
{
 
    OriginChunk chunk_hdr;
    int len = origin.length();
    chunk_hdr.chunkHdr.chunkSz[0] = len & 0xff;
    chunk_hdr.chunkHdr.chunkSz[1] = (len >> 8) & 0xff;
    chunk_hdr.chunkHdr.chunkSz[2] = (len >> 16) & 0xff;
    chunk_hdr.chunkHdr.chunkSz[3] = (len >> 24) & 0xff;

    fout.write((char*)&chunk_hdr, sizeof(chunk_hdr));

    for(int i = 0; i < len; i++ )
        fout.write((char*)&origin[i], 1);

    if (mVerbose)
        printf("Origin chunk 0000 with text '%s' written.\n", origin.c_str());

    return true;
}

bool UEFCodec::addBytes2Vector(Bytes& v, Byte bytes[], int n)
{
    for (int i = 0; i < n; i++) {
        Byte b = bytes[i];
        v.push_back(b);
    }
    return true;
}

bool UEFCodec::updateAtomBlockState(CHUNK_TYPE chunkType, double duration)
{
    switch (chunkType) {
    case CHUNK_TYPE::GAP_CHUNK:
    {
        if (
            mBlockState == BLOCK_STATE::READING_DATA || // Normally a gap comes after reading the data
            mBlockState == BLOCK_STATE::READING_GAP // In case the gap is split ito several gap chunks
        ) {
            if (!firstBlock) { // gap between blocks are saved as an attribute of the previous block
                mCurrentBlockInfo.block_gap = duration;
                pushCurrentBlock();
            }
            else {
                firstBlock = false;              
            }
            mBlockState = BLOCK_STATE::READING_GAP;
        }
        else
            return false;

        break;
    }
    case CHUNK_TYPE::CARRIER_CHUNK:
    {
        if ( // A lead tone?
            mBlockState == BLOCK_STATE::READING_GAP || // Normally a lead tone comes after a gap
            mBlockState == BLOCK_STATE::READING_DATA // Also allow lead tone without previous gap
        ) {     
            mCurrentBlockInfo.lead_tone_cycles = duration * mBaseFrequency * 2;
            mBlockState = BLOCK_STATE::READING_CARRIER;
            mCurrentBlockInfo.start = mCurrentTime;
        }
        else if ( // Must be a micro tone (separating block header and block data)
            mBlockState == BLOCK_STATE::READING_HDR // A micro tone must come after reading the header
            ) {
            mCurrentBlockInfo.micro_tone_cycles = duration * mBaseFrequency * 2;
            mBlockState = BLOCK_STATE::READING_MICRO_TONE;
        }
        else
            return false;
        break;
    }
    case CHUNK_TYPE::DATA_CHUNK:
    {
        if ( // Block header?
            mBlockState == BLOCK_STATE::READING_CARRIER || // Normally the block header comes after the lead tone
            mBlockState == BLOCK_STATE::READING_HDR // In case the block header is being split into several data chunks
            ) {
            mBlockState = BLOCK_STATE::READING_HDR;
        }
        else if ( // Must be block data then
            mBlockState == BLOCK_STATE::READING_MICRO_TONE || // Normally the block data comes after the micro tone
            mBlockState == BLOCK_STATE::READING_DATA // In case the block data is split into several data chunks
            ) {
            mBlockState = BLOCK_STATE::READING_DATA;
            mCurrentBlockInfo.end = mCurrentTime; // Last block data read will give the time when the block ends
        }
        else {
            return false;
        }
        break;
    }

    default:
    {
        cout << "Undefined chunk type " << (int)chunkType << "\n";
        return false;
    }

    }
}

/*
 * Expected chunks for a file are:
 *  (<carrier> <data 0xaa> <carrier> | <carrier with dummy byte>) <data> {<carrier> <data>} <carrier> <gap>
 *
*/
bool UEFCodec::updateBBMBlockState(CHUNK_TYPE chunkType, double duration, int preludeCycles)
{
    switch (chunkType) {
    case CHUNK_TYPE::GAP_CHUNK: // Gap between complete tape files
    {
        if (
            mBlockState == BLOCK_STATE::READING_CARRIER_PRELUDE || // A gap comes after reading the last block only
            mBlockState == BLOCK_STATE::READING_CARRIER // For a carrier tone including a dummy byte or a trailer carrier 
        ) {
            // Regret pushing the current block info for the last block before the trailer carrier
            // The lead tone cycles recorded were actually trailer tone cycles
            int trailer_tone_cycles = mCurrentBlockInfo.lead_tone_cycles;
            pullCurrentBlock();
            mCurrentBlockInfo.trailer_tone_cycles = trailer_tone_cycles;
            mCurrentBlockInfo.block_gap = duration;
            firstBlock = true;   
            mBlockState = BLOCK_STATE::READING_GAP;
            pushCurrentBlock(); 
        }
        else
            return false;

        break;
    }
    case CHUNK_TYPE::CARRIER_CHUNK:
    // Either a lead carrier for block (includes a dummy byte a first block) or a trailer carrier (last block)
    {
        if ( // A lead carrier?
            mBlockState == BLOCK_STATE::READING_GAP // A carrier after a gap signals the end of a file and the start of a new one
            ) {
            mBlockState = BLOCK_STATE::READING_CARRIER_PRELUDE;
            mCurrentBlockInfo.start = mCurrentTime;
            mCurrentBlockInfo.prelude_lead_tone_cycles = duration * mBaseFrequency * 2;
            firstBlock = true;
        } else if (mBlockState == BLOCK_STATE::READING_HDR_DATA) { // A tone signals start of next block (except for last block) 
            if (!firstBlock) {
                pushCurrentBlock(); // save previous block's info (if it was the last block this pushing will be reverted when a gap is detected)
            }
            mCurrentBlockInfo.lead_tone_cycles = duration * mBaseFrequency * 2;
            mBlockState = BLOCK_STATE::READING_CARRIER_PRELUDE;     
        } else if (mBlockState == BLOCK_STATE::READING_DUMMY_BYTE) { // A tone follows after a dummy byte for the first block
            mBlockState = BLOCK_STATE::READING_CARRIER_POSTLUDE;
            mCurrentBlockInfo.lead_tone_cycles = duration * mBaseFrequency * 2;
        }
        else
            return false;
        break;
    }
    case CHUNK_TYPE::CARRIER_DUMMY_CHUNK:
        // Either a lead carrier for block (includes a dummy byte a first block) or a trailer carrier (last block)
    {
        if ( // A lead carrier?
            mBlockState == BLOCK_STATE::READING_GAP // A carrier after a gap signals the end of a file and the start of a new one
            ) {
             mBlockState = BLOCK_STATE::READING_CARRIER_WITH_DUMMY;
            mCurrentBlockInfo.start = mCurrentTime;
            mCurrentBlockInfo.prelude_lead_tone_cycles = preludeCycles;
            mCurrentBlockInfo.lead_tone_cycles = duration * mBaseFrequency * 2;
            firstBlock = false;
        }
        else
            return false;
        break;
    }
    case CHUNK_TYPE::DATA_CHUNK: // Either a dummy byte in a lead carrier for the first block of a file or block header+data
    {
        if (mBlockState == BLOCK_STATE::READING_CARRIER_PRELUDE && firstBlock) {
            mBlockState = BLOCK_STATE::READING_DUMMY_BYTE;
            firstBlock = false;
        }
        else if ( // Block header & data?
            mBlockState == BLOCK_STATE::READING_CARRIER_POSTLUDE ||
            mBlockState == BLOCK_STATE::READING_CARRIER_PRELUDE ||
            mBlockState == BLOCK_STATE::READING_CARRIER_WITH_DUMMY
            ) {
            mBlockState = BLOCK_STATE::READING_HDR_DATA;
        }
        else if (mBlockState == BLOCK_STATE::READING_HDR_DATA) {
            // Continuation of header/data - no action required
        }
        else {
            printf("Unexpected data chunck for block state %s!\n", _BLOCK_STATE(mBlockState));
            return false;
        }
        break;
    }

    default:
    {
        cout << "Undefined chunk type " << (int)chunkType << "\n";
        return false;
    }

    }
}
bool UEFCodec::updateBlockState(CHUNK_TYPE chunkType, double duration, int preludeCycles)
{
    if (mTargetMachine == TargetMachine::UNKNOWN_TARGET)
        return true;

    if (mTargetMachine <= BBC_MASTER)
        updateBBMBlockState(chunkType, duration, preludeCycles);
    else
        updateAtomBlockState(chunkType, duration);

    mCurrentTime += duration;

    if (mVerbose && mBlockState != mPrevBlockState)
        cout << "Block reading state changed to " << _BLOCK_STATE(mBlockState) << "...\n";

    mPrevBlockState = mBlockState;

    return true;
}
