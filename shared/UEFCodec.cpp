#include <gzstream.h>
#include <stdlib.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string.h>
#include "UEFCodec.h"
#include "CommonTypes.h"
#include "AtomBlockTypes.h"
#include "Logging.h"
#include "Utility.h"
#include "AtomBlockTypes.h"
#include "BitTiming.h"
#include <cmath>
#include <cstdint>



using namespace std;

bool UEFCodec::setStdOut(ostream* fout)
{
    mFout = fout;
    return true;
}

int UEFCodec::getPhaseShift()
{
    return mPhase;
}

double UEFCodec::getBaseFreq()
{
    return mBaseFrequency;
}

bool UEFCodec::decodeFloat(Byte encoded_val[4], double& decoded_val)
{

    /* decode mantissa */
    int m = encoded_val[0] | (encoded_val[1] << 8) | ((encoded_val[2] & 0x7f) | 0x80) << 16;

    decoded_val = (double) m;
    decoded_val = (double) ldexp(decoded_val, -23);

    /* decode exponent */
    int exp = (((encoded_val[2] & 0x80) >> 7) | (encoded_val[3] & 0x7f) << 1) - 127;
    decoded_val = (double) ldexp(decoded_val, exp);

    /* flip sign if necessary */
    if (encoded_val[3] & 0x80)
        decoded_val = -decoded_val;
  
    return true;
}

bool UEFCodec::encodeFloat(double val, Byte encoded_val[4])
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
    double m = (double) frexp(val, &exp);
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

UEFCodec::UEFCodec(Logging logging, TargetMachine mTargetMachine) :
    mDebugInfo(logging), mTargetMachine(mTargetMachine)
{
    if (mTargetMachine <= BBC_MASTER)
        mTapeTiming = bbmTiming;
    else if (mTargetMachine == ACORN_ATOM)
        mTapeTiming = atomTiming;
    else // Have some default timing initially even if a machine hasn't been selected
        mTapeTiming = defaultTiming;
 

    if (mTargetMachine != UNKNOWN_TARGET) {
        // Only change the UEF file default values if a target is specified
        mBaudRate = mTapeTiming.baudRate;
        mBaseFrequency = mTapeTiming.baseFreq;
        mPhase = mTapeTiming.phaseShift;
    }

    BitTiming bit_timing = BitTiming(mBitTiming.fS, mBaseFrequency, mBaudRate, mTargetMachine);
    mBitTiming = bit_timing;
}

UEFCodec::UEFCodec(bool useOriginalTiming, Logging logging, TargetMachine mTargetMachine):
    mUseOriginalTiming(useOriginalTiming),mDebugInfo(logging), mTargetMachine(mTargetMachine)
{
    if (mTargetMachine <= BBC_MASTER)
        mTapeTiming = bbmTiming;
    else if (mTargetMachine == ACORN_ATOM)
        mTapeTiming = atomTiming;
    else// Have some default timing initially even if a machine hasn't been selected
        mTapeTiming = defaultTiming;

    if (mTargetMachine != UNKNOWN_TARGET) {
        // Only change the UEF file defaults values if a target is specified
        mBaudRate = mTapeTiming.baudRate;
        mBaseFrequency = mTapeTiming.baseFreq;
        mPhase = mTapeTiming.phaseShift;
    }

    BitTiming bit_timing = BitTiming(mBitTiming.fS, mBaseFrequency, mBaudRate, mTargetMachine);
    mBitTiming = bit_timing;
}

bool UEFCodec::setTapeTiming(TapeProperties tapeTiming)
{
    mTapeTiming = tapeTiming;

    mBaudRate = mTapeTiming.baudRate;
    mBaseFrequency = mTapeTiming.baseFreq;
    mPhase = mTapeTiming.phaseShift;

    return true;
}

bool UEFCodec::encodeBBM(TapeFile& tapeFile)
{

    FileBlockIter file_block_iter = tapeFile.blocks.begin();
    TargetMachine file_block_type = BBC_MODEL_B;

    int block_no = 0;
    int n_blocks = (int)tapeFile.blocks.size();

    // Default values for block timing (if mUseOriginalTiming is true the recorded block timing will instead by used later on)
    int prelude_lead_tone_cycles = mTapeTiming.nomBlockTiming.firstBlockPreludeLeadToneCycles;
    double lead_tone_duration = mTapeTiming.nomBlockTiming.firstBlockLeadToneDuration; // let first block have a longer lead tone
    double other_block_lead_tone_duration = mTapeTiming.nomBlockTiming.otherBlockLeadToneDuration;// lead tone duration of all other blocks (2 s expected here but Atomulator needs 4 s)
    double last_block_gap = mTapeTiming.nomBlockTiming.lastBlockGap;
    double trailer_tone_duration = mTapeTiming.nomBlockTiming.trailerToneDuration;
 
    //
    // Write data-dependent part of UEF file
    //

    if (mDebugInfo.verbose) {
        *mFout << "\nEncoding data-dependent part of BBC Micro Tape File...\n";
    }

    while (file_block_iter < tapeFile.blocks.end()) {

        // Write a lead tone for the block - including a dummy byte if it is the first block
        if (mUseOriginalTiming && tapeFile.validTiming) {
            prelude_lead_tone_cycles = file_block_iter->preludeToneCycles;
            lead_tone_duration = (file_block_iter->leadToneCycles) / (2 * mBaseFrequency);
        }

        if (firstBlock) {
            double prelude_lead_tone_duration = prelude_lead_tone_cycles / (2 * mBaseFrequency);
            int postlude_lead_tone_cycles = (int) round(lead_tone_duration * (2 * mBaseFrequency));
            if (!writeCarrierChunkwithDummyByte(*mTapeFile_p, prelude_lead_tone_cycles, postlude_lead_tone_cycles)) {
                *mFout << "Failed to write " << prelude_lead_tone_cycles <<
                    " cycles prelude tone and dummy byte 0xaa followed by " << lead_tone_duration << " s postlude tone\n";
            }
        }
        else {
            if (!writeCarrierChunk(*mTapeFile_p, lead_tone_duration)) {
                *mFout << "Failed to write " << mBaseFrequency * 2 <<
                    " lead tone of duration " << lead_tone_duration << "\n";
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


        int data_len = file_block_iter->size; // get data length

        

        // Preamble
        Bytes preamble;
        Word dummy_CRC;
        preamble.push_back(0x2a);
        if (!writeSimpleDataChunk(*mTapeFile_p, preamble, dummy_CRC)) {
            *mFout << "Failed to write block preamble data chunk for block #" << block_no << "\n";
        }

        Bytes header_data;

        // Initialise header CRC (starts from file name)
        Word header_CRC = 0;

        // Store header bytes
        if (!file_block_iter->encodeTapeBlockHdr(header_data)) {
            *mFout << "Failed to encode header for block #" << block_no << "\n";
            return false;
        }

        // Write header excluding the CRC
        if (!writeComplexDataChunk(*mTapeFile_p, 8, 'N', 1, header_data, header_CRC)) {
            *mFout << "Failed to write block header data chunk for block #" << block_no << "\n";
        }

        // Write the header CRC chunk
        Bytes header_CRC_bytes;
        dummy_CRC = 0;
        header_CRC_bytes.push_back(header_CRC / 256);
        header_CRC_bytes.push_back(header_CRC % 256);
        if (!writeSimpleDataChunk(*mTapeFile_p, header_CRC_bytes, dummy_CRC)) {
            *mFout << "Failed to write header CRC data chunk for block #" << block_no << "\n";
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
        if (!writeComplexDataChunk(*mTapeFile_p, 8, 'N', 1, block_data, data_CRC)) {
            *mFout << "Failed to write block header data chunk\n";
        }

        // Write the data CRC
        Bytes data_CRC_bytes;
        data_CRC_bytes.push_back(data_CRC / 256);
        data_CRC_bytes.push_back(data_CRC % 256);
        if (!writeSimpleDataChunk(*mTapeFile_p, data_CRC_bytes, dummy_CRC)) {
            *mFout << "Failed to write data CRC chunk\n";
        }


        // --------------------------------------------------------------------------
        //
        // ---------------- End of block data chunk -------------------------------



        // For last block, add trailer carrier and gap
        if (block_no == n_blocks - 1) {

            // Write a trailer carrier
            if (mUseOriginalTiming && tapeFile.validTiming)
                trailer_tone_duration = (file_block_iter->trailerToneCycles) / (2 * mBaseFrequency);
            if (!writeCarrierChunk(*mTapeFile_p, trailer_tone_duration)) {
                *mFout << "Failed to write trailer lead tone chunk\n";
            }

            // write a gap
            if (mUseOriginalTiming && tapeFile.validTiming)
                last_block_gap = file_block_iter->blockGap;
            if (!writeFloatPrecGapChunk(*mTapeFile_p, last_block_gap)) {
                *mFout << "Failed to write " << last_block_gap << "s of gap for last block\n";
            }
        };
        

        file_block_iter++;


        block_no++;

    }

    return true;
}

bool UEFCodec::encodeAtom(TapeFile& tapeFile)
{
    
    FileBlockIter file_block_iter = tapeFile.blocks.begin();

    int block_no = 0;
    int n_blocks = (int)tapeFile.blocks.size();

    // Default values for block timing (if mUseOriginalTiming is true the recored block timing will instead by used later on)
    double first_block_lead_tone_duration = mTapeTiming.nomBlockTiming.firstBlockLeadToneDuration;// lead tone duration of first block 
    double other_block_lead_tone_duration = mTapeTiming.nomBlockTiming.otherBlockLeadToneDuration;// lead tone duration of all other blocks (2 s expected here but Atomulator needs 4 s)
    double data_block_micro_lead_tone_duration = mTapeTiming.nomBlockTiming.microLeadToneDuration; //  micro lead tone (separatiing block header and block data) duration
    double block_gap = mTapeTiming.nomBlockTiming.blockGap;
    double last_block_gap = mTapeTiming.nomBlockTiming.lastBlockGap;

    double lead_tone_duration = first_block_lead_tone_duration; // let first block have a longer lead tone

    //
    // Write data-dependent part of UEF file
    //


    while (file_block_iter < tapeFile.blocks.end()) {

        // Write a lead tone for the block
        if (mUseOriginalTiming && tapeFile.validTiming)
            lead_tone_duration = (file_block_iter->leadToneCycles) / (2 * mBaseFrequency);
        if (!writeCarrierChunk(*mTapeFile_p, lead_tone_duration)) {
            *mFout << "Failed to write " << mBaseFrequency * 2 << " Hz lead tone of duration " << lead_tone_duration << "s\n";
        }
        

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

        Bytes header_data;

        Byte preamble[4] = { 0x2a, 0x2a, 0x2a, 0x2a };              // store preamble
        addBytes2Vector(header_data, preamble, 4);

        // Store header bytes
        if (!file_block_iter->encodeTapeBlockHdr(header_data)) {
            *mFout << "Failed to encode header for block #" << block_no << "\n";
            return false;
        }


        if (!writeComplexDataChunk(*mTapeFile_p, 8, 'N', -1, header_data, CRC)) {
            *mFout << "Failed to write block header data chunk\n";
        }


        // --------------------------------------------------------------------------
        //
        // ---------------- End of block header chunk -------------------------------


        // Add micro tone between header and data
        if (mUseOriginalTiming && tapeFile.validTiming)
            data_block_micro_lead_tone_duration = file_block_iter->microToneCycles / (2 * mBaseFrequency);
        if (!writeCarrierChunk(*mTapeFile_p, data_block_micro_lead_tone_duration)) {
            *mFout << "Failed to write " << 2 * mBaseFrequency << " Hz micro lead tone of duration " << 
                data_block_micro_lead_tone_duration << "s\n";
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
        if (!writeComplexDataChunk(*mTapeFile_p, 8, 'N', -1, block_data, CRC)) {
            *mFout << "Failed to write block header data chunk\n";
        }


        //
        // Write CRC
        //

        // Write CRC chunk header
        Bytes CRC_data;
        Word dummy_CRC = 0;
        CRC_data.push_back(CRC & 0xff);
        if (!writeComplexDataChunk(*mTapeFile_p, 8, 'N', -1, CRC_data, dummy_CRC)) {
            *mFout << "Failed to write CRC data chunk\n";
        }



        // --------------------------------------------------------------------------
        //
        // ---------------- End of block data chunk -------------------------------



        // Write 1 security cycle
        /*
        if (!writeSecurityCyclesChunk(*mTapeFile_p, 1, 'W', 'P', { 1 })) {
            *mFout << "Failed to write security bytes\n";
        }
        */

        // Write a gap at the end of the block
        if (block_no == n_blocks - 1)
            block_gap = last_block_gap;
        if (mUseOriginalTiming && tapeFile.validTiming)
            block_gap = file_block_iter->blockGap;
        if (!writeFloatPrecGapChunk(*mTapeFile_p, block_gap)) {
            *mFout << "Failed to write " << block_gap << "s of block gap\n";
        }

        file_block_iter++;


        block_no++;

    }

    return true;
}

bool UEFCodec::openTapeFile(string& filePath)
{
    mTapeFilePath = filePath;
    mTapeFile_p = new ogzstream(filePath.c_str());
    if (!mTapeFile_p->good()) {
        cout << "Can't write to UEF file '" << mTapeFilePath << "\n";
        return false;
    }

    mFirstFile = true;

    return true;
}

bool UEFCodec::closeTapeFile()
{
    mTapeFile_p->close();

    if (!mTapeFile_p->good()) {
        cout << "Failed writing to file '" << mTapeFilePath << "' for compressed output\n";
        return false;
    }

    delete mTapeFile_p;

    return true;
}


bool UEFCodec::encode(TapeFile& tapeFile, string& filePath)
{
    if (!openTapeFile(filePath))
        return false;

    string tapeFileName;

    if (mDebugInfo.verbose)
        *mFout << "Encoding program '" << tapeFileName << "' into UEF file...\n\n";

    encode(tapeFile);

    if (!closeTapeFile())
        return false;

    if (mDebugInfo.verbose)
        *mFout << "\nDone encoding program '" << tapeFileName << "' as an UEF file...\n\n";

    return true;
}

bool UEFCodec::writeFileIndepPart(TapeFile& tapeFile)
{


    string tapeFileName = "???";

    //
    // Write data-independent part of UEF file
    //

    /*
     * Timing parameters
     *
     * Based on default or actually recorded timing
     */

    double first_block_gap = mTapeTiming.nomBlockTiming.firstBlockGap;

    int baudrate = mTapeTiming.baudRate;
    if (mUseOriginalTiming && tapeFile.validTiming)
        baudrate = tapeFile.baudRate;

    // Intialise base frequency and phase shift with values from Tape Timing
    mBaseFrequency = mTapeTiming.baseFreq;
    mPhase = mTapeTiming.phaseShift;


    // Header: UEF File!
    UefHdr hdr;
    mTapeFile_p->write((char*)&hdr, sizeof(hdr));


    // Add origin
    string origin = "AtomTapeUtils";
    if (!writeOriginChunk(*mTapeFile_p, origin)) {
        *mFout << "Failed to write origin chunk with string '" << origin << "'\n";
    }

    // Write target
    if (!writeTargetChunk(*mTapeFile_p)) {
        *mFout << "Failed to write target chunk with target " << _TARGET_MACHINE(mTargetMachine) << "\n";
    }


    // Baudrate
    if (!writeBaudrateChunk(*mTapeFile_p)) {
        *mFout << "Failed to write baudrate chunk with baudrate " << baudrate << "\n";
    }


    // Phaseshift
    if (!writePhaseChunk(*mTapeFile_p)) {
        *mFout << "Failed to write phaseshift chunk with phase shift " << mPhase << " degrees\n";
    }

    // Write base frequency
    if (!writeBaseFrequencyChunk(*mTapeFile_p)) {
        *mFout << "Failed to encode base frequency " << mBaseFrequency << " Hz as double\n";
    }

    // Write an intial gap before the file starts
    if (!writeFloatPrecGapChunk(*mTapeFile_p, first_block_gap)) {
        *mFout << "Failed to write " << first_block_gap << "s gap\n";
    }

    return true;
}

/*
 * Encode TAP File structure into already open UEF file
 */
bool UEFCodec::encode(TapeFile& tapeFile)
{
    if (tapeFile.blocks.empty()) 
        return false;

    if (mFirstFile) {
        mFirstFile = false;
        mTargetMachine = tapeFile.header.targetMachine;
        if (!writeFileIndepPart(tapeFile))
            return false;
    }

    if (mTargetMachine <= BBC_MASTER) {
        encodeBBM(tapeFile);
    }
    else if (mTargetMachine <= ACORN_ATOM) {
        encodeAtom(tapeFile);
    }
    else {
        *mFout << "Unknown target machine " << hex << mTargetMachine << "\n";
        return false;
    }  

    return true;

}

bool UEFCodec::validUefFile(string& uefFileName)
{
    igzstream fin(uefFileName.c_str());

    // Is it a compressed file?
    if (!fin.good()) {
        return false;
    }

    // Read first part of UEF File to check that it is indeed an UEF file
    UefHdr hdr;
    fin.read((char*)&hdr, sizeof(hdr));
    if (strcmp(hdr.uefTag, "UEF File!") != 0) {
        fin.close();
        return false;
    }

    // Close file
    fin.close();
    /*
    if (!fin.eof())
        return false;
    */

    // Success
    return true;
}

bool UEFCodec::readUefFile(string& uefFileName)
{
    mUefData.clear();

    igzstream fin(uefFileName.c_str());

    // Is it a compressed file?
    if (!fin.good()) {
        return false;
    }

    // Read first part of UEF File to check that it is indeed an UEF file
    UefHdr hdr;
    fin.read((char*)&hdr, sizeof(hdr));
    if (strcmp(hdr.uefTag, "UEF File!") != 0) {
        fin.close();
        return false;
    }


    // Read all data
    char c;
    while (fin.read(&c, 1))
        mUefData.push_back(c);

    // Close file
    fin.close();


    if (!fin.eof())
        return false;

    mUefDataIter = mUefData.begin();

    // Success
    return true;

}

bool UEFCodec::readBytes(Byte* dst, int n)
{
    int i;
    for (i = 0; i < n && mUefDataIter < mUefData.end(); *(dst + i++) = *mUefDataIter++);

    return (i == n);
}

bool UEFCodec::processChunk(ChunkInfo &chunkInfo)
{

    ChunkHdr chunk_hdr;

    chunkInfo.init();

    // Extract chunk hdr from byte stream
    if (!readBytes((Byte*)  & chunk_hdr, sizeof(chunk_hdr))) {
        return false;
    }
 
    if (mUefDataIter < mUefData.end()) {

            uint16_t chunk_id = chunk_hdr.chunkId[0] + chunk_hdr.chunkId[1] * 256;
            uint32_t chunk_sz = chunk_hdr.chunkSz[0] + (chunk_hdr.chunkSz[1] << 8) + (chunk_hdr.chunkSz[2] << 16) + (chunk_hdr.chunkSz[3] << 24);

            chunkInfo.chunkId = chunk_id;
            chunkInfo.chunkSz = chunk_sz;

            if (mDebugInfo.verbose)
                *mFout << "\n" << Utility::encodeTime(getTime()) << ":\n";

            switch (chunk_id) {

            case TARGET_CHUNK: // Target machine
            {
                if (chunk_sz != 1) {
                    *mFout << "Size of target machine chunk 0005 has an incorrect chunk size " << chunk_sz << " (should have been 1)\n";
                    return false;
                }
                TargetMachineChunk chunk;
                if (!readBytes(&chunk.targetMachine, sizeof(chunk.targetMachine))) return false;
                if (_TARGET_MACHINE(chunk.targetMachine) == "???") {
                    *mFout << "Invalid target machine " << (int)chunk.targetMachine << "\n";
                }
                mTargetMachine = (TargetMachine)chunk.targetMachine;
                if (mDebugInfo.verbose)
                    *mFout << "Target machine chunk 0005 of size " << chunk_sz << " and specifying target " << _TARGET_MACHINE(mTargetMachine) << ".\n";

                chunkInfo.chunkInfoType = ChunkInfoType::IGNORE;

                break;
            }

            case FP_GAP_CHUNK: // Floating-point gap
            {
                if (chunk_sz != 4) {
                    *mFout << "Size of Floating-point gap chunk 0116 has an incorrect chunk size " << chunk_sz << " (should have been 4)\n";
                    return false;
                }
                FPGapChunk gap_chunk;
                if (!readBytes(gap_chunk.gap, sizeof(gap_chunk.gap))) return false;
                double gap;

                if (!decodeFloat(gap_chunk.gap, gap)) {
                    *mFout << "Failed to decode IEEE 754 gap\n";
                }
                if (mDebugInfo.verbose)
                    *mFout << "Floating-point gap chunk 0116 of size " << chunk_sz << " and specifying a gap of " << gap << " s.\n";

                chunkInfo.chunkInfoType = ChunkInfoType::GAP;
                chunkInfo.data1_fp = gap;

                mTime += gap;

                break;
            }

            case BASE_FREQ_CHUNK: // Base frequency
            {
                if (chunk_sz != 4) {
                    *mFout << "Size of Base frequency chunk 0113 has an incorrect chunk size " << chunk_sz << " (should have been 4)\n";;
                    return false;
                }
                BaseFreqChunk chunk;
                if (!readBytes(chunk.frequency, sizeof(chunk.frequency))) return false;
                if (!decodeFloat(chunk.frequency, mBaseFrequency)) {
                    *mFout << "Failed to decode IEEE 754 gap\n";
                }
                if (mDebugInfo.verbose)
                    *mFout << "Base frequency chunk 0113 of size " << chunk_sz << " and specifying a frequency of " << mBaseFrequency << ".Hz\n";

                // Update bit timing as impacted by the change of the base frequency
                BitTiming bit_timing = BitTiming(mBitTiming.fS, mBaseFrequency, mBaudRate, mTargetMachine);
                mBitTiming = bit_timing;

                chunkInfo.chunkInfoType = ChunkInfoType::BASE_FREQ;
                chunkInfo.data1_fp = mBaseFrequency;

                break;
            }

            case DATA_ENCODING_FORMAT_CHANGE_CHUNK: // Data Encoding Format Chunk 0117: baudrate = value
            {
                if (chunk_sz != 2) {
                    *mFout << "Size of Data Encoding Format chunk 0117 has an incorrect chunk size " << chunk_sz << " (should have been 2)\n";;
                    return false;
                }
                DataEncodingFormatChangeChunk baudrate_chunk;
                if (!readBytes(baudrate_chunk.baudRate, sizeof(baudrate_chunk.baudRate))) return false;
                mBaudRate = (baudrate_chunk.baudRate[0] + (baudrate_chunk.baudRate[1] << 8));

                
                if (mDebugInfo.verbose)
                    *mFout << "Data Encoding Format chunk 0117 of size " << chunk_sz << " and baudrate of " << mBaudRate << ".\n";

                // Update bit timing as impacted by the change of the Baudrate
                BitTiming bit_timing = BitTiming(mBitTiming.fS, mBaseFrequency, mBaudRate, mTargetMachine);
                mBitTiming = bit_timing;

                chunkInfo.chunkInfoType = ChunkInfoType::BAUDRATE;
                chunkInfo.data2_i = mBaudRate;

                break;
            }
            case PHASE_CHUNK: // Phase Shift Change Chunk 0115: phase shift = value
            {
                if (chunk_sz != 2) {
                    *mFout << "Size of Phase Shift change chunk 0115 has an incorrect chunk size " << chunk_sz << " (should have been 2)\n";;
                    return false;
                }
                PhaseChunk phase_chunk;
                phase_chunk.chunkHdr = chunk_hdr;
                if (!readBytes(phase_chunk.phase_shift, sizeof(phase_chunk.phase_shift))) return false;
                mPhase = (phase_chunk.phase_shift[0] + (phase_chunk.phase_shift[1] << 8));
                if (mDebugInfo.verbose)
                    *mFout << "Phase change chunk 0115 of size " << chunk_sz << " and specifying a 1/2 cycle of " << mPhase << " degrees.\n";

                chunkInfo.chunkInfoType = ChunkInfoType::PHASE;
                chunkInfo.data2_i = mPhase;

                break;
            }

            case INTEGER_GAP_CHUNK: // Integer Gap Chunk 0112: gap = n/(2*base frequency) s
            {
                if (chunk_sz != 2) {
                    *mFout << "Size of Integer gap chunk 0112 has an incorrect chunk size  " << chunk_sz << " (should have been 2)\n";;
                    return false;
                }
                IntegerGapChunk gap_chunk;
                gap_chunk.chunkHdr = chunk_hdr;
                if (!readBytes(gap_chunk.gap, sizeof(gap_chunk.gap))) return false;
                double gap = double(gap_chunk.gap[0] + (gap_chunk.gap[1] << 8)) / (mBaseFrequency * 2);
                if (mDebugInfo.verbose)
                    *mFout << "Integer gap chunk 0112 of size " << chunk_sz << " and specifying a duration of " << gap << " s.\n";

                chunkInfo.chunkInfoType = ChunkInfoType::GAP;
                chunkInfo.data1_fp = gap;

                mTime += gap;

                break;
            }

            case CARRIER_TONE_CHUNK: // Carrier Tone Chunk 0110: duration = value / (mBaseFrequency * 2)
            {
                if (chunk_sz != 2) {
                    *mFout << "Size of Carrier tone chunk 0110 has an incorrect chunk size  " << chunk_sz << " (should have been 2)\n";;
                    return false;
                }
                CarrierChunk tone_chunk;
                tone_chunk.chunkHdr = chunk_hdr;
                if (!readBytes(tone_chunk.duration, sizeof(tone_chunk.duration))) return false;
                double tone_duration = double(tone_chunk.duration[0] + (tone_chunk.duration[1] << 8)) / (mBaseFrequency * 2);
                if (mDebugInfo.verbose)
                    *mFout << "Carrier tone chunk 0110 of size " << chunk_sz << " and specifying a duration of " << tone_duration << " s.\n";

                chunkInfo.chunkInfoType = ChunkInfoType::CARRIER;
                chunkInfo.data1_fp = tone_duration;

                mTime += tone_duration;

                break;
            }

            case CARRIER_TONE_WITH_DUMMY_BYTE: // Carrier Chunk with dummy byte 0111: durations in carrier tone cycles
            {
                if (chunk_sz != 4) {
                    *mFout << "Size of Carrier tone with dummy byte chunk 0111 has an incorrect chunk size " << chunk_sz << " (should have been 2)\n";;
                    return false;
                }
                CarrierChunkDummy tone_chunk;
                tone_chunk.chunkHdr = chunk_hdr;
                if (!readBytes(&tone_chunk.duration[0], sizeof(tone_chunk.duration))) return false;
                int first_cycles = Utility::bytes2uint((Byte*)&tone_chunk.duration[0], 2, true);
                double first_duration = first_cycles / (2 * mBaseFrequency);
                int following_cycles = Utility::bytes2uint((Byte*)&tone_chunk.duration[2], 2, true);
                double following_duration = following_cycles / (2 * mBaseFrequency);

                if (mDebugInfo.verbose)
                    *mFout << "Carrier tone chunk with dummy byte 0111 of size " << chunk_sz << " and specifying " << first_cycles <<
                    " cycles of carrier followed by a dummy byte 0xaa, and ending with " << following_duration << "s of carrier.\n";

                chunkInfo.chunkInfoType = ChunkInfoType::CARRIER_DUMMY;
                chunkInfo.data1_fp = first_duration;
                chunkInfo.data2_i = following_cycles;
                chunkInfo.data.push_back(0xaa);

                mTime += first_duration + following_duration;

                break;
            }

            case SECURITY_CHUNK: // Security cycles
            {
                SecurityCyclesChunkHdr hdr;
                hdr.chunkHdr = chunk_hdr;

                // Get #cycles
                if (!readBytes(&hdr.nCycles[0], sizeof(hdr.nCycles))) return false;
                int n_cycles = hdr.nCycles[0] + (hdr.nCycles[1] << 8);

                // Get first & last pulse info
                if (!readBytes(&hdr.firstPulse, sizeof(hdr.firstPulse))) return false;
                if (!readBytes(&hdr.lastPulse, sizeof(hdr.lastPulse))) return false;

                // Get cycle bytes        
                int n_cycle_bytes = (int)ceil((double)n_cycles / 8);
                Bytes cycles;
                string cycle_bytes = "[";
                for (int i = 0; i < n_cycle_bytes; i++) {
                    Byte b;
                    if (!readBytes(&b, 1)) return false;
                    if (i < n_cycle_bytes - 1)
                        cycle_bytes += to_string(b) + ", ";
                    else
                        cycle_bytes += to_string(b) + "]";
                    cycles.push_back(b);
                }
                string cycle_string = decode_security_cycles(hdr, cycles);     

                if (mDebugInfo.verbose) {
                    *mFout << "Security cycles chunk 0114 of size " << chunk_sz << " specifying " << n_cycles;
                    *mFout << " with format " << hdr.firstPulse << hdr.lastPulse << " and cycle bytes " << cycle_bytes << ".\n";
                }

                chunkInfo.chunkInfoType = ChunkInfoType::IGNORE;

                mTime += security_cycles_length(cycle_string);

                break;
            }

            case ORIGIN_CHUNK: // Origin Chunk 0000
            {
                string origin = "";
                for (uint32_t pos = 0; pos < chunk_sz; pos++) {
                    Byte c;
                    if (!readBytes(&c, 1))
                        return false;
                    origin += (char) c;
                }
                if (mDebugInfo.verbose)
                    *mFout << "Origin chunk 000 of size " << chunk_sz << " and text '" << origin << "'.\n";

                chunkInfo.chunkInfoType = ChunkInfoType::IGNORE;

                break;
            }

            case IMPLICIT_DATA_BLOCK_CHUNK: // Implicit Data Block Chunk 0100; default start & stop bits
            {
                int count = 0;
                chunkInfo.chunkInfoType = ChunkInfoType::DATA;
                chunkInfo.data1_fp = chunk_sz * mBitTiming.F2CyclesPerByte / (2 * mBaseFrequency);
                for (uint32_t i = 0; i < chunk_sz; i++) {
                    Byte byte;
                    if (!readBytes(&byte, 1)) return false;
                    chunkInfo.data.push_back(byte);
                }
                if (mTargetMachine == ACORN_ATOM)
                    chunkInfo.dataEncoding = atomDefaultDataEncoding;
                else
                    chunkInfo.dataEncoding = bbmDefaultDataEncoding; // defaults to BBC Machine 8N1 encoding

                mTime += chunkInfo.data1_fp;

                if (mDebugInfo.verbose) {
                    *mFout << "Implicit Data Block chunk 0100 of size " << chunk_sz << ":";
                    BytesIter di = chunkInfo.data.begin();
                    Utility::logData(mFout, 0x0, di, (int) chunkInfo.data.size());
                }
                break;
            }

            case DEFINED_TAPE_FORMAT_DATA_BLOCK_CHUNK: // Data Block Chunk 0104
            {
                DefinedTapeFormatDBChunkHdr hdr;
                hdr.chunkHdr = chunk_hdr;
                if (!readBytes(&hdr.bitsPerPacket, sizeof(hdr.bitsPerPacket))) return false;
                if (!readBytes(&hdr.parity, sizeof(hdr.parity))) return false;
                if (!readBytes(&hdr.stopBitInfo, sizeof(hdr.stopBitInfo))) return false;
                int n_stop_bits = (hdr.stopBitInfo >= 0x80 ? hdr.stopBitInfo - 256 : hdr.stopBitInfo);
                bool extra_short_wave = false;
                if (hdr.stopBitInfo >= 0x80)
                    extra_short_wave = true;

                chunkInfo.chunkInfoType = ChunkInfoType::DATA;
                chunkInfo.data1_fp = chunk_sz * mBitTiming.F2CyclesPerByte / (2 * mBaseFrequency);
                for (uint32_t i = 0; i < chunk_sz - 3; i++) {
                    Byte byte;
                    if (!readBytes(&byte, 1)) return false;
                    chunkInfo.data.push_back(byte);
                }
                chunkInfo.dataEncoding.bitsPerPacket = hdr.bitsPerPacket;
                chunkInfo.dataEncoding.parity = (hdr.parity == 'N' ? Parity::NO_PAR : (hdr.parity == 'O' ? Parity::ODD : Parity::EVEN));
                chunkInfo.dataEncoding.nStopBits = abs(n_stop_bits);
                chunkInfo.dataEncoding.extraShortWave = extra_short_wave;

                if (mDebugInfo.verbose) {
                    *mFout << "Defined Tape Format Data Block chunk 0104 of size " << chunk_sz << " and with encoding " <<
                        dec << (int) hdr.bitsPerPacket  << hdr.parity << n_stop_bits << ":";
                    BytesIter di = chunkInfo.data.begin();
                    Utility::logData(mFout, 0x0, di, (int) chunkInfo.data.size());
                }


                mTime += chunkInfo.data1_fp;

                break;
            }

            default: // Unsupported chunk
            {
                // Still store the chunk data
                chunkInfo.chunkInfoType = ChunkInfoType::IGNORE;
                for (uint32_t i = 0; i < chunk_sz - 3; i++) {
                    Byte byte;
                    if (!readBytes(&byte, 1)) return false;
                    chunkInfo.data.push_back(byte);
                }

                if (mDebugInfo.verbose) {
                    *mFout << "Unsupported chunk " << chunkInfo.chunkId << " of size " << chunk_sz << " and with data:\n";
                    BytesIter di = chunkInfo.data.begin();
                    Utility::logData(mFout, 0x0, di, (int)chunkInfo.data.size());
                }

                break;
            }

            }

    }
    else {
        // End of tape
        return false;
    }

    return true;
}

void UEFCodec::consume_bytes(int &n, Bytes &data) {
   
    if (mRemainingData.size() == 0)
        return;

    int n_read = min(n, (int) mRemainingData.size());
    for (int i = 0; i < n_read; data.push_back(mRemainingData[i++])); // consume n bytes
    mRemainingData.erase(
        mRemainingData.begin(), mRemainingData.begin() + n_read
    ); // remove them from saved data

    n -= n_read;
}

bool UEFCodec::readFromDataChunk(int n, Bytes& data)
{
    ChunkInfo chunk_info;
    bool end_of_file = false;
    data.clear();

    // Special handling if a data chunk is requested
    double t_start = mTime;

    // Collect data chunks until the requested n bytes has been collected (or it fails)
    int n_remaining = n;
    while (n_remaining > 0) {
        consume_bytes(n_remaining, data);
        if (n_remaining > 0) {
            if (!processChunk(chunk_info) ||  chunk_info.chunkInfoType != DATA || end_of_file)
                return false;
            for (int i = 0; i < chunk_info.data.size(); mRemainingData.push_back(chunk_info.data[i++]));
        }
        // Update time (overrides the update made in processChunk - needed as only part of the data's chunk might be read)
        mTime = t_start + data.size() * mBitTiming.F2CyclesPerByte / (2 * mBaseFrequency);

    }
    return true;
  
}

bool UEFCodec::detectCarrier(double& waitingTime, double& duration)
{
    double dummy_duration;
    return detectCarrier(waitingTime, duration, dummy_duration, true, false);
}

bool UEFCodec::detectCarrier(double& duration)
{
    double dummy_duration, dummy_waiting_time;
    return detectCarrier(dummy_waiting_time, duration, dummy_duration, false, false);
}

bool UEFCodec::detectCarrierWithDummyByte(double& waitingTime, double& preludeDuration, double& postLudeDuration)
{
    return detectCarrier(waitingTime, preludeDuration, postLudeDuration, true, true);
}

bool UEFCodec::detectCarrierWithDummyByte(double& preludeDuration, double& postLudeDuration)
{
    double dummy_waiting_time;
    return detectCarrier(dummy_waiting_time, preludeDuration, postLudeDuration, false, true);
}

bool UEFCodec::detectCarrier(double &waitingTime, double& preludeDuration, double & postLudeDuration, bool skipData, bool acceptDummy)
{
    ChunkInfo chunk_info;
    bool end_of_file = false;

    double t_start = mTime;

    // Wait until carrier chunk is found (only allow DATA chunks in the sequence if skipData is true)
    while (
        processChunk(chunk_info) &&
        chunk_info.chunkInfoType != CARRIER &&
        !(acceptDummy && chunk_info.chunkInfoType == CARRIER_DUMMY) &&
        !(!skipData && chunk_info.chunkInfoType == DATA)
        );

    // If stopped at a DATA chunk, then either end of tape or skipping data was not allowed => STOP
    if (chunk_info.chunkInfoType == DATA || chunk_info.chunkInfoType == ChunkInfoType::UNKNOWN)
        return false;

    double duration;
    preludeDuration = chunk_info.data1_fp;
    if (chunk_info.chunkInfoType == CARRIER_DUMMY) {
        postLudeDuration = (double) chunk_info.data2_i / (mBaseFrequency * 2);
        duration = preludeDuration + postLudeDuration;
    } 
    else {
        postLudeDuration = -1;
        duration = preludeDuration;
    }

    waitingTime = mTime - t_start - duration;

    return true;
}
bool UEFCodec::detectGap(double& duration)
{
    ChunkInfo chunk_info;
    bool end_of_file = false;

    // Wait until carrier chunk is found (only allow DATA chunks in the sequence if skipData is true)
    while (
        processChunk(chunk_info) &&
        chunk_info.chunkInfoType != GAP && chunk_info.chunkInfoType != DATA && chunk_info.chunkInfoType != CARRIER
        );

    if (chunk_info.chunkInfoType == DATA || chunk_info.chunkInfoType == CARRIER)
        return false;

    return true;
}

//
// Decode UEF file but only print its content rather than converting it a Tape File
//
bool UEFCodec::inspect(string& uefFileName)
{
    if (!readUefFile(uefFileName)) {
        *mFout << "Failed to read UEF file '" << uefFileName << "'\n";
    }
    
    // Get all data chunk bytes
    Bytes data;
    ChunkInfo chunk_info;
    while (processChunk(chunk_info)) {
        switch (chunk_info.chunkInfoType) {
        case ChunkInfoType::CARRIER_DUMMY:
            data.push_back(0xaa);
            break;
        case ChunkInfoType::DATA:
            for (int i = 0; i < chunk_info.data.size(); data.push_back(chunk_info.data[i++]));
            break;
        default:
            break;
        }
    }

    // Output all collected data bytes
    BytesIter data_iter = data.begin();
    Utility::logData(mFout, 0x0, data_iter, (int)data.size());

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
     if (mDebugInfo.verbose)
         *mFout << "\n" << Utility::encodeTime(getTime()) << ":\n";

     BaseFreqChunk chunk;
     if (!encodeFloat(mBaseFrequency, chunk.frequency)) {
         *mFout << "Failed to encode base frequency " << mBaseFrequency << " Hz as double.\n";
         return false;
     }
     fout.write((char*) &chunk, sizeof(chunk));

     if (mDebugInfo.verbose)
         *mFout << "Base frequency chunk 0113 specifying " << mBaseFrequency << " Hz written.\n";

     return true;
}

 bool UEFCodec::writeTargetChunk(ogzstream& fout)
 {
     if (mDebugInfo.verbose)
         *mFout << "\n" << Utility::encodeTime(getTime()) << ":\n";

     TargetMachineChunk chunk;
     chunk.targetMachine = mTargetMachine;
     fout.write((char*)&chunk, sizeof(chunk));

     if (mDebugInfo.verbose)
         *mFout << "Target machine chunk 0005 specifying a target " << _TARGET_MACHINE(mTargetMachine) << " written.\n";

     return true;
 }

 // Write a double-precision gap 
bool UEFCodec::writeFloatPrecGapChunk(ogzstream&fout, double duration)
{
    if (mDebugInfo.verbose)
        *mFout << "\n" << Utility::encodeTime(getTime()) << ":\n";

     FPGapChunk chunk;

    if (!encodeFloat(duration, chunk.gap)) {
        *mFout << "Failed to encode gap as double of duration " << duration << "s\n";
        return false;
    }
    fout.write((char*)&chunk, sizeof(chunk));

    if (mDebugInfo.verbose)
        *mFout << "Gap 0116 chunk specifying a gap of " << duration << "s written.\n";

    mTime += duration;


    return true;
}

// Write an integer-precision gap
bool UEFCodec::writeIntPrecGapChunk(ogzstream& fout, double duration)
{
    if (mDebugInfo.verbose)
        *mFout << "\n" << Utility::encodeTime(getTime()) << ":\n";

    IntegerGapChunk chunk;
    int gap = (int) round(duration * mBaseFrequency * 2);
    chunk.gap[0] = gap & 0xff;
    chunk.gap[1] = (gap >> 8) & 0xff;
    fout.write((char*) &chunk, sizeof(chunk));

    if (mDebugInfo.verbose)
        *mFout << "Gap 0112 chunk specifying a gap of " << duration << "s written.\n";

    mTime += duration;

    return true;
}

// Write security cycles
bool UEFCodec::writeSecurityCyclesChunk(ogzstream& fout, int nCycles, Byte firstPulse, Byte lastPulse, Bytes cycles)
{
    if (mDebugInfo.verbose)
        *mFout << "\n" << Utility::encodeTime(getTime()) << ":\n";

    SecurityCyclesChunkHdr hdr;


    // Set up header
    hdr.nCycles[0] = nCycles & 0xff;
    hdr.nCycles[1] = (nCycles >> 8) & 0xff;
    hdr.nCycles[2] = (nCycles >> 16) & 0xff;
    hdr.firstPulse = firstPulse;
    hdr.lastPulse = lastPulse;
    int chunk_sz = (int) cycles.size() + 5;
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

    if (mDebugInfo.verbose)
        *mFout << "Security cycles chunk 0114 of size " << chunk_sz <<
        " specifying " << nCycles << " security cycles with format '" << (char) hdr.firstPulse << (char) hdr.lastPulse <<
        "' and cycle bytes '" << cycle_bytes << "' written\n";

    mTime += security_cycles_length(cycle_string);

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

// Determine duration of security cycles from decoded string of security cycles
double UEFCodec::security_cycles_length(string cycles)
{
    double t = 0;
    for (int i = 0; i < cycles.length(); i++) {
        if (cycles[0] == '0' || cycles[i] == 'H')
            t += 1 / (2 * mBaseFrequency); // one 1/2 cycle of base frequency OR 1 cycle of carrier frequency
        else if (cycles[0] == '1')
            t += 1 / (4 * mBaseFrequency); // one 1/2 cycle of carrier frequency 
        else if (cycles[i] == 'L') // one cycle of base frequency
            t += 1 / mBaseFrequency;
        else // error
            return -1;
    }
    return t;
}

// write baudrate 
bool UEFCodec::writeBaudrateChunk(ogzstream&fout)
{
    if (mDebugInfo.verbose)
        *mFout << "\n" << Utility::encodeTime(getTime()) << ":\n";

    DataEncodingFormatChangeChunk chunk;
    chunk.baudRate[0] = mBaudRate & 0xff;
    chunk.baudRate[1] = mBaudRate >> 8;
    fout.write((char*)&chunk, sizeof(chunk));

    if (mDebugInfo.verbose)
        *mFout << "Baudrate chunk 0117 specifying a baud rate of " << mBaudRate << " written.\n";

    return true;
}

// Write phase_shift chunk
bool UEFCodec::writePhaseChunk(ogzstream &fout)
{
    if (mDebugInfo.verbose)
        *mFout << "\n" << Utility::encodeTime(getTime()) << ":\n";

    PhaseChunk chunk;
    chunk.phase_shift[0] = mPhase & 0xff;
    chunk.phase_shift[1] = mPhase >> 8;
    fout.write((char*)&chunk, sizeof(chunk));

    if (mDebugInfo.verbose)
        *mFout << "Phase chunk 0115 specifying a phase_shift of " << mPhase << " degrees written.\n";
    return true;
}

bool UEFCodec::writeCarrierChunk(ogzstream &fout, double duration)
{
    if (mDebugInfo.verbose)
        *mFout << "\n" << Utility::encodeTime(getTime()) << ":\n";

    // Write a high tone
    CarrierChunk chunk;
    int cycles = (int) round(duration * mBaseFrequency * 2);
    chunk.duration[0] = cycles % 256;
    chunk.duration[1] = cycles / 256;
    fout.write((char*)&chunk, sizeof(chunk));

    if (mDebugInfo.verbose)
        *mFout << "Carrier chunk 0110 specifying a " << duration << "s tone written.\n";
 
    mTime += duration;

    return true;
}

bool UEFCodec::writeCarrierChunkwithDummyByte(ogzstream& fout, int firstCycles, int followingCycles)
{
    if (mDebugInfo.verbose)
        *mFout << "\n" << Utility::encodeTime(getTime()) << ":\n";

    // Write a high tone
    CarrierChunkDummy chunk;
    double duration = followingCycles / (2 * mBaseFrequency);
    chunk.duration[0] = firstCycles % 256;
    chunk.duration[1] = firstCycles / 256;
    chunk.duration[2] = followingCycles % 256;
    chunk.duration[3] = followingCycles / 256;
    fout.write((char*)&chunk, sizeof(chunk));

    if (mDebugInfo.verbose)
        *mFout << "Carrier chunk 0111 specifying " << firstCycles << " cycles of a carrier followed by a dummy byte 0xaa and ending with " <<
        duration << "s of carrier written.\n";

    mTime += (double) (firstCycles + followingCycles) / (2 * mBaseFrequency);

    return true;
}

bool UEFCodec::writeComplexDataChunk(ogzstream &fout, Byte bitsPerPacket, Byte parity, Byte stopBitInfo, Bytes data, Word & CRC)
{
    if (mDebugInfo.verbose)
        *mFout << "\n" << Utility::encodeTime(getTime()) << ":\n";

    DefinedTapeFormatDBChunkHdr chunk;
    int block_size = (int) data.size() + 3; // add 3 bytes for the #bits/packet, parity & stop bit
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
        if (bitsPerPacket == 7)
            b = b & 0x7f;
        FileBlock::updateCRC(mTargetMachine, CRC, b);
        fout.write((char*)&b, sizeof(b));
    }

    if (mDebugInfo.verbose) {
        *mFout << "Data chunk 0104 of size " << dec << block_size << " and encoding " << 
            (int)bitsPerPacket << parity << (int) stopBitInfo << " written:";
        BytesIter di = data.begin();
        Utility::logData(mFout, 0x0, di, (int) data.size());
    }

    mTime += block_size * mBitTiming.F2CyclesPerByte / (2 * mBaseFrequency);;

    return true;
}

bool UEFCodec::writeSimpleDataChunk(ogzstream &fout, Bytes data, Word &CRC)
{
    if (mDebugInfo.verbose)
        *mFout << "\n" << Utility::encodeTime(getTime()) << ":\n";

    ImplicitDataBlockChunkHdr chunk;
    int block_size = (int) data.size();
    chunk.chunkHdr.chunkSz[0] = block_size & 0xff;
    chunk.chunkHdr.chunkSz[1] = (block_size >> 8) & 0xff;
    chunk.chunkHdr.chunkSz[2] = (block_size >> 16) & 0xff;
    chunk.chunkHdr.chunkSz[3] = (block_size >> 23) & 0xff;
    fout.write((char*)&chunk, sizeof(chunk));

    BytesIter data_iter = data.begin();
    for (int i = 0; i < block_size; i++) {
        Byte b = *data_iter++;
        FileBlock::updateCRC(mTargetMachine, CRC, b);
        fout.write((char*)&b, sizeof(b));
    }

    if (mDebugInfo.verbose) {
        *mFout << "Data chunk 0100 of size " << dec << block_size << " written:";
        BytesIter di = data.begin();
        Utility::logData(mFout, 0x0, di, (int) data.size());
    }

    mTime += block_size * mBitTiming.F2CyclesPerByte / (2 * mBaseFrequency);

    return true;
}

// Write origin chunk
bool UEFCodec::writeOriginChunk(ogzstream &fout, string origin)
{
    if (mDebugInfo.verbose)
        *mFout << "\n" << Utility::encodeTime(getTime()) << ":\n";

    OriginChunk chunk_hdr;
    int len = (int) origin.length();
    chunk_hdr.chunkHdr.chunkSz[0] = len & 0xff;
    chunk_hdr.chunkHdr.chunkSz[1] = (len >> 8) & 0xff;
    chunk_hdr.chunkHdr.chunkSz[2] = (len >> 16) & 0xff;
    chunk_hdr.chunkHdr.chunkSz[3] = (len >> 24) & 0xff;

    fout.write((char*)&chunk_hdr, sizeof(chunk_hdr));

    for(int i = 0; i < len; i++ )
        fout.write((char*)&origin[i], 1);

    if (mDebugInfo.verbose)
        *mFout << "Origin chunk 0000 with text '" << origin << "' written.\n";

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

double UEFCodec::getTime()
{
    return mTime;
}

bool UEFCodec::rollback()
{
    if (checkpoints.size() == 0)
        return false;

    // Get reference to last  element
    UEFChkPoint cp = checkpoints.back(); 

    // Restore data iter & time
    mUefDataIter = cp.pos;
    mTime = cp.time;

    // Restore the buffered chunk data
    mRemainingData.clear();
    for (int i = 0; i < cp.bufferedData.size(); mRemainingData.push_back(cp.bufferedData[i++]));

    // Dispose of the last element
    checkpoints.pop_back();

    return true;
}

// Remove checkpoint (without rolling back)
bool UEFCodec::regretCheckpoint()
{
    if (checkpoints.size() == 0)
        return false;

    // Remove last checkpoint element
    checkpoints.pop_back();

    return true;
}

bool UEFCodec::checkpoint()
{
    // Make a copy of the buffered chunk data
    Bytes remaining_data;
    for (int i = 0; i < mRemainingData.size(); remaining_data.push_back(mRemainingData[i++]));

    // Create a checkpoint element
    UEFChkPoint cp(mUefDataIter, mTime, remaining_data);

    // Add the element to the checkpoints
    checkpoints.push_back(cp);

    return true;
}
