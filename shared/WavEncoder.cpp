#include <fstream>
#include <filesystem>
#include <iostream>
#include "CommonTypes.h"
#include "WavEncoder.h"
#include "../shared/WaveSampleTypes.h"
#include "../shared/PcmFile.h"
#include "../shared/Debug.h"
#include "../shared/Utility.h"
#include "../shared/TapeProperties.h"

using namespace std;

bool WavEncoder::init()
{
    if (!Utility::setBitTiming(mTapeTiming.baudRate, mTargetMachine, mStartBitCycles,
        mLowDataBitCycles, mHighDataBitCycles, mStopBitCycles)
        ) {
        throw invalid_argument("Unsupported baud rate " + mTapeTiming.baudRate);
    }
    mHighSamples = (double)mFS / F2_FREQ;
    mLowSamples = (double)mFS / F1_FREQ;
    if (mVerbose) {
        cout << "Baudrate: " << dec << mTapeTiming.baudRate << " Hz\n";
        cout << "mStartBitCycles: " << mStartBitCycles << "\n";
        cout << "mLowDataBitCycles: " << mLowDataBitCycles << "\n";
        cout << "mHighDataBitCycles: " << mHighDataBitCycles << "\n";
        cout << "mStopBitCycles: " << mStopBitCycles << "\n";
        cout << "\n";
    }

    return true;
}

WavEncoder::WavEncoder(
    bool useOriginalTiming, int sampleFreq, bool verbose, TargetMachine targetMachine
): mUseOriginalTiming(useOriginalTiming), mFS(sampleFreq), mVerbose(verbose), mTargetMachine(targetMachine)
{
    if (targetMachine <= BBC_MASTER)
        mTapeTiming = bbmTiming;
    else
        mTapeTiming = atomTiming;
    if (!init()) {
        cout << "Failed to initialise the WavEncoder\n";
    };
}

WavEncoder::WavEncoder(int sampleFreq, bool verbose, TargetMachine targetMachine) : mFS(sampleFreq), mVerbose(verbose), mTargetMachine(targetMachine)
{
    if (!targetMachine)
        mTapeTiming = atomTiming;
    else
        mTapeTiming = bbmTiming;
    if (!init()) {
        cout << "Failed to initialise the WavEncoder\n";
    }
}

bool WavEncoder::setTapeTiming(TapeProperties tapeTiming)
{
    mTapeTiming = tapeTiming;
    if (mVerbose) {

        Utility::logTapeTiming(mTapeTiming);
    }
    return true;
}

bool WavEncoder::encode(TapeFile& tapeFile, string& filePath)
{
    if (tapeFile.fileType <= BBC_MASTER)
         return encodeBBM(tapeFile, filePath);
    else
        return encodeBBM(tapeFile, filePath);
}

/*
 * Encode BBC Micro TAP File structure as WAV file
 */
bool WavEncoder::encodeBBM(TapeFile& tapeFile, string& filePath)

{
    TargetMachine file_block_type = BBC_MODEL_B;

    int prelude_lead_tone_cycles = mTapeTiming.nomBlockTiming.firstBlockPreludeLeadToneCycles;
    double lead_tone_duration = mTapeTiming.nomBlockTiming.firstBlockLeadToneDuration;
    double other_block_lead_tone_duration = mTapeTiming.nomBlockTiming.otherBlockLeadToneDuration;
    double trailer_tone_duration = mTapeTiming.nomBlockTiming.trailerToneDuration;
    double first_block_gap = mTapeTiming.nomBlockTiming.firstBlockGap;
    double last_block_gap = mTapeTiming.nomBlockTiming.lastBlockGap;

    double high_tone_freq = mTapeTiming.baseFreq * 2;


    if (tapeFile.blocks.empty()) {
        cout << "Cannot encode an empty TAP File!\n";
        return false;
    }


    FileBlockIter block_iter = tapeFile.blocks.begin();

    if (mVerbose)
        cout << "\nEncoding program '" << tapeFile.blocks[0].blockName() << "' as a WAV file...\n\n";


    int block_no = 0;
    int n_blocks = (int)tapeFile.blocks.size();

    // Encode initial gap before first block
    if (!writeGap(first_block_gap)) {
        printf("Failed to encode a gap of %f s\n", first_block_gap);
    }

    if (mVerbose)
        cout << first_block_gap << " s GAP\n";

    while (block_iter < tapeFile.blocks.end()) {

        
        if (mUseOriginalTiming) {
            prelude_lead_tone_cycles = block_iter->preludeToneCycles;
            lead_tone_duration = (block_iter->leadToneCycles) / high_tone_freq;
            mPhase = block_iter->phaseShift;
        }

        // Write a lead tone (including a dummy byte for the first lock)
        if (block_no == 0) {
            double prelude_lead_tone_duration = prelude_lead_tone_cycles / high_tone_freq;
            if (!writeTone(prelude_lead_tone_duration)) { // Normally 4 initial cycles
                printf("Failed to write lead tone of duration %f s\n", lead_tone_duration);
            }
            writeByte(0xaa); // dummy byte
        }
        if (!writeTone(lead_tone_duration)) { // remaining cycles
            printf("Failed to write lead tone of duration %f s\n", lead_tone_duration);
        }

        if (mVerbose) {
            if (block_no > 0)
                cout << "BLOCK " << block_no << ": LEAD TONE " << lead_tone_duration << " s : ";
            else
                cout << "BLOCK 0: PRELUDE " << prelude_lead_tone_cycles << " cycles : DUMMY BYTE : POSTLUDE " << lead_tone_duration << " s : ";
        }

        // Change lead tone duration for remaining blocks
        lead_tone_duration = other_block_lead_tone_duration;


        // Initialise header CRC
        mCRC = 0;

        // --------------------- start of block header  ------------------------
        //
        // Write block header including preamble
        //
        // <preamble:1> <block name:1-10> <load adr:4> <exec adr:4> <block no:2> <block len:2> <block flag:1> <next adr:4>
        // 
        // --------------------------------------------------------------------------

        Bytes header_data;

        // Store preamble
        header_data.push_back(0x2a);
        

        // Store header bytes
        if (!Utility::encodeTapeHdr(file_block_type, *block_iter, block_no, n_blocks, header_data)) {
            cout << "Failed to encode header bytes for block #" << block_no << "\n";
            return false;
        }

        // Encode the header bytes
        BytesIter hdr_iter = header_data.begin();
        while (hdr_iter < header_data.end())
            writeByte(*hdr_iter++);

        //
        // Write the header CRC
        //

        // Encode CRC byte
        Word header_CRC = mCRC;
        if (!writeByte(header_CRC / 256) || !writeByte(header_CRC % 256)) {
            printf("Failed to encode header CRC!%s\n", "");
            return false;
        }


        if (mVerbose)
            cout << "HDR+CRC : ";


        // Initialise data CRC
        mCRC = 0;


        // --------------------------------------------------------------------------
        //
        // ---------------- End of block header chunk -------------------------------


        // --------------------- start of block data + CRC chunk ------------------------
         //
         // Write block data as one chunk
         //
         // --------------------------------------------------------------------------

        // Write block data
        BytesIter bi = block_iter->data.begin();
        Bytes block_data;
        while (bi < block_iter->data.end())
            block_data.push_back(*bi++);

        // Encode the block data bytes
        BytesIter data_iter = block_data.begin();
        while (data_iter < block_data.end())
            writeByte(*data_iter++);


        //
        // Write the data CRC
        //

        Word data_CRC = mCRC;
        if (!writeByte(data_CRC / 256) || !writeByte(data_CRC % 256)) {
            printf("Failed to encode data CRC!%s\n", "");
            return false;
        }


        if (mVerbose)
            cout << "Data+CRC : ";



        // --------------------------------------------------------------------------
        //
        // ---------------- End of block data chunk -------------------------------

        // Write trailer tone and a gap if it is the last block
        if (block_no == n_blocks - 1) {

            if (mUseOriginalTiming)
                trailer_tone_duration = (block_iter->trailerToneCycles) / high_tone_freq;

            // Write trailer tone
            if (!writeTone(trailer_tone_duration)) {
                printf("Failed to write lead tone of duration %f s\n", trailer_tone_duration);
                return false;
            }

            if (mVerbose)
                cout << trailer_tone_duration << " s TRAILER TONE : ";

            // Write the gap
            double block_gap = last_block_gap;
            if (mUseOriginalTiming)
                block_gap = block_iter->blockGap;

            if (!writeGap(block_gap)) {
                printf("Failed to encode a gap of %f s\n", block_gap);
                return false;
            }

            if (mVerbose)
                cout << block_gap << " s GAP";

        }

        if (mVerbose)
            cout << "\n";

        block_iter++;


        block_no++;

    }

    if (mVerbose)
        cout << mSamples.size() << " samples created from Tape File!\n";

    if (mSamples.size() == 0) {
        cout << "No samples could be created from Tape File!\n";
        return false;
    }

    // Write samples to WAV file
    Samples samples_v[] = { mSamples };
    if (!writeSamples(filePath, samples_v, 1, mFS, mVerbose)) {
        printf("Failed to write samples!%s\n", "");
        return false;
    }


    // Clear samples to secure that future encodings start without any initial samples
    mSamples.clear();

    if (mVerbose)
        cout << "\nDone encoding program '" << tapeFile.blocks[0].blockName() << "' as a WAV file...\n\n";

    return true;
}

/*
 * Encode Acorn Atom TAP File structure as WAV file
 */
bool WavEncoder::encodeAtom(TapeFile &tapeFile, string& filePath)
{
    TargetMachine file_block_type = ACORN_ATOM;

    double lead_tone_duration = mTapeTiming.nomBlockTiming.firstBlockLeadToneDuration;
    double other_block_lead_tone_duration = mTapeTiming.nomBlockTiming.otherBlockLeadToneDuration;
    double data_block_micro_lead_tone_duration = mTapeTiming.nomBlockTiming.microLeadToneDuration;
    double first_block_gap = mTapeTiming.nomBlockTiming.firstBlockGap;
    double block_gap = mTapeTiming.nomBlockTiming.blockGap;
    double last_block_gap = mTapeTiming.nomBlockTiming.lastBlockGap;

    double high_tone_freq = mTapeTiming.baseFreq * 2;

    if (tapeFile.blocks.empty()) {
        cout << "Cannot encode an empty TAP File!\n";
        return false;
    }


    FileBlockIter block_iter = tapeFile.blocks.begin();

    if (mVerbose)
        cout << "\nEncoding program '" << tapeFile.blocks[0].blockName() << "' as a WAV file...\n\n";
    

    int block_no = 0;
    int n_blocks = (int)tapeFile.blocks.size();

    // Encode initial gap before first block
    if (!writeGap(first_block_gap)) {
        printf("Failed to encode a gap of %f s\n", block_gap);
    }

    if (mVerbose)
        cout << first_block_gap << " s GAP\n";

    while (block_iter < tapeFile.blocks.end()) {

        // Write a lead tone for the block
        if (mUseOriginalTiming) {
            lead_tone_duration = (block_iter->leadToneCycles) / high_tone_freq;
            mPhase = block_iter->phaseShift;
        }
        if (!writeTone(lead_tone_duration)) {
            printf("Failed to write lead tone of duration %f s\n", lead_tone_duration);
        }

        if (mVerbose)
            cout << "BLOCK " << block_no << ": LEAD TONE " << lead_tone_duration << " s : ";

        // Change lead tone duration for remaining blocks
        lead_tone_duration = other_block_lead_tone_duration;


        // Initialise CRC

        mCRC = 0;

        // --------------------- start of block header  ------------------------
        //
        // Write block header including preamble
        //
        // block header: <flags:1> <block_no:2> <length-1:1> <exec addr:2> <load addr:2>
        // 
        // --------------------------------------------------------------------------

        Bytes header_data;

        // store preamble
        for (int i = 0; i < 4; i++)
            header_data.push_back(0x2a);

        // Store header bytes
        if (!Utility::encodeTapeHdr(file_block_type, *block_iter, block_no, n_blocks, header_data)) {
            cout << "Failed to encode header for block #" << block_no << "\n";
            return false;
        }


        // Encode the header bytes
        BytesIter hdr_iter = header_data.begin();
        while (hdr_iter < header_data.end())
            writeByte(*hdr_iter++);


        if (mVerbose)
            cout << "HDR : ";
     

        // --------------------------------------------------------------------------
        //
        // ---------------- End of block header chunk -------------------------------


        // Add micro tone between header and data
        if (mUseOriginalTiming)
            data_block_micro_lead_tone_duration = (block_iter->microToneCycles) / high_tone_freq;
        if (!writeTone(data_block_micro_lead_tone_duration)) {
            printf("Failed to write micro lead tone of duration %f s\n", data_block_micro_lead_tone_duration);
            return false;
        }

        
        if (mVerbose)
            cout << data_block_micro_lead_tone_duration << " s micro tone : ";
         


        // --------------------- start of block data + CRC chunk ------------------------
         //
         // Write block data as one chunk
         //
         // --------------------------------------------------------------------------

        // Write block data
        BytesIter bi = block_iter->data.begin();
        Bytes block_data;
        while (bi < block_iter->data.end())
            block_data.push_back(*bi++);

        // Encode the block data bytes
        BytesIter data_iter = block_data.begin();
        while (data_iter < block_data.end())
            writeByte(*data_iter++);




        


        //
        // Write CRC
        //

        // Encode CRC byte

        if (!writeByte(mCRC & 0xff)) {
            printf("Failed to encode CRC!%s\n", "");
            return false;
        }

        
        if (mVerbose)
            cout << "Data+CRC : ";
         
        

        // --------------------------------------------------------------------------
        //
        // ---------------- End of block data chunk -------------------------------

 

        // Write a gap at the end of the block
        if (mUseOriginalTiming)
            block_gap = (block_iter->blockGap);
        else if (block_no == n_blocks - 1)
            block_gap = last_block_gap;

        if (!writeGap(block_gap)) {
            printf("Failed to encode a gap of %f s\n", block_gap);
            return false;
        }

        
        if (mVerbose)
            cout << block_gap << " s GAP\n";
         

        block_iter++;


        block_no++;

    }

    if (mVerbose)
        cout << mSamples.size() << " samples created from Tape File!\n";

    if (mSamples.size() == 0) {
        cout << "No samples could be created from Tape File!\n";
        return false;
    }


    // Write samples to WAV file
    Samples samples_v[] = { mSamples };
    if (!writeSamples(filePath, samples_v, 1, mFS, mVerbose)) {
        printf("Failed to write samples!%s\n", "");
        return false;
    }


    // Clear samples to secure that future encodings start without any initial samples
    mSamples.clear();

    if (mVerbose)
        cout << "\nDone encoding program '" << tapeFile.blocks[0].blockName() << "' as a WAV file...\n\n";

	return true;
}


bool WavEncoder::writeByte(Byte byte)
{
    if (!writeStartBit())
        return false;

    Byte b = byte;
    for (int i = 0; i < 8; i++) {
        if (!writeDataBit(b & 0x1)) {
            printf("Failed to encode '%d' data bit #%d\n", b & 0x1, i);
            return false;
        }
        b = b >> 1;
    }

    if (!writeStopBit())
        return false;

    Utility::updateCRC(mTargetMachine, mCRC, byte);

    return true;

}


bool WavEncoder::writeDataBit(int bit)
{
    int n_cycles;
    bool high_frequency;

    if (bit != 0) {
        n_cycles = mHighDataBitCycles;
        high_frequency = true;
    }
    else {
        n_cycles = mLowDataBitCycles;
        high_frequency = false;
    }  

    
    if (!writeCycle(high_frequency, n_cycles)) {
        printf("Failed to encode '%d' data bit #%d\n", high_frequency, bit);
        return false;
    }


    return true;
}

bool WavEncoder::writeStartBit()
{
    int n_cycles = mStartBitCycles;

    if (!writeCycle(false, n_cycles)) {
        printf("Failed to encode start bit%s\n", "");
        return false;
    }

    return true;
}

bool WavEncoder::writeStopBit()
{
    int n_cycles = mStopBitCycles;

    if (!writeCycle(true, n_cycles)) {
        printf("Failed to encode stop bit%s\n", "");
        return false;
    }


    return true;
}

bool WavEncoder::writeTone(double duration)
{
    int n_cycles = (int) round((double) duration * F2_FREQ);

    if (!writeCycle(true, n_cycles)) {
        printf("Failed to encode %lf duration (%d cycles) tone.\n", duration, n_cycles);
        return false;
    }

    /*
    if (mVerbose)
        printf("%lf s (%d cycles) tone written!\n", duration, n_cycles);
    */

    return true;
}

bool WavEncoder::writeGap(double duration)
{
    unsigned n_samples = (int) round(duration * mFS);

    if (n_samples == 0)
        return true;

    for (unsigned s = 0; s < n_samples; s++) {
        mSamples.push_back(0);
    }

    return true;
}

bool WavEncoder::writeCycle(bool highFreq, unsigned n)
{
    if (n == 0)
        return true;

    const double PI = 3.14159265358979323846;
    int n_samples;
    if (highFreq) {
        n_samples = (int) round(mHighSamples * n);
        /*
        if (mVerbose)
            printf("%d cycles of F2\n", n);
         */
    } 
    else {
        /*
        if (mVerbose)
            printf("%d cycles of F1\n", n);
        */
        n_samples = (int) round(mLowSamples * n);
    }

    double half_cycle = ((mPhase + 180) % 360) * PI / 180;

    double f = 2 * n * PI / n_samples;
    for (int s = 0; s < n_samples; s++) {
        Sample y = (Sample) round(sin(s * f + half_cycle) * mMaxSampleAmplitude);
        mSamples.push_back(y);
    }
    return true;
}
