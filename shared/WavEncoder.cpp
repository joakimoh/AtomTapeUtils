#include <fstream>
#include <filesystem>
#include <iostream>
#include "CommonTypes.h"
#include "WavEncoder.h"
#include "WaveSampleTypes.h"
#include "PcmFile.h"
#include "Logging.h"
#include "Utility.h"
#include "TapeProperties.h"
#include <cmath>

using namespace std;

WavEncoder::WavEncoder(
    bool useOriginalTiming, int sampleFreq, TapeProperties tapeTiming, Logging logging, TargetMachine targetMachine
): mUseOriginalTiming(useOriginalTiming), mBitTiming(sampleFreq, tapeTiming.baseFreq, tapeTiming.baudRate, targetMachine),
    mDebugInfo(logging), mTargetMachine(targetMachine)
{
    if (targetMachine <= BBC_MASTER)
        mTapeTiming = bbmTiming;
    else if (targetMachine == ACORN_ATOM)
        mTapeTiming = atomTiming;
    else
        mTapeTiming = defaultTiming;

}

WavEncoder::WavEncoder(int sampleFreq, TapeProperties tapeTiming, Logging logging, TargetMachine targetMachine) :
    mBitTiming(sampleFreq, tapeTiming.baseFreq, tapeTiming.baudRate, targetMachine), mDebugInfo(logging), mTargetMachine(targetMachine)
{
    if (!targetMachine)
        mTapeTiming = atomTiming;
    else if (targetMachine == ACORN_ATOM)
        mTapeTiming = atomTiming;
    else
        mTapeTiming = defaultTiming;

}

bool WavEncoder::openTapeFile(string& filePath)
{
    mSamples.clear();

    mTapeFilePath = filePath;

    return true;
}

bool WavEncoder::closeTapeFile()
{
    if (mSamples.size() == 0)
        return false;

    // Write samples to WAV file
    if (!writeSamples(mTapeFilePath)) {
        printf("Failed to write samples!%s\n", "");
        return false;
    }

    return true;
}

bool WavEncoder::encode(TapeFile& tapeFile, string& filePath)
{
    
    // Initialise encoding
    if (!openTapeFile(filePath))
        return false;

    // Get samples from tape file
    encode(tapeFile);

    // Write them to file and close file
    if (!closeTapeFile())
        return false;

    if (mDebugInfo.verbose)
        cout << "\nDone encoding program '" << tapeFile.blocks[0].blockName() << "' as a WAV file...\n\n";

    return true;
}

// Get samples from tape file and add it to the total set of samples
bool WavEncoder::encode(TapeFile& tapeFile)
{
    if (tapeFile.metaData.targetMachine <= BBC_MASTER)
        return encodeBBM(tapeFile);
    else
        return encodeAtom(tapeFile); 
}

/*
 * Encode BBC Micro TAP File structure as WAV file
 */
bool WavEncoder::encodeBBM(TapeFile& tapeFile)
{
    int initial_no_samples = (int) mSamples.size(); // zero unless many tape files are encoded into one and the same set of samples

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

    if (mDebugInfo.verbose)
        cout << "\nEncoding program '" << tapeFile.blocks[0].blockName() << "' as a WAV file...\n\n";


    int block_no = 0;
    int n_blocks = (int)tapeFile.blocks.size();

    // Encode initial gap before first block
    if (!writeGap(first_block_gap)) {
        printf("Failed to encode a gap of %f s\n", first_block_gap);
    }

    if (mDebugInfo.verbose)
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
            writeByte(0xaa, bbmDefaultDataEncoding); // dummy byte
        }
        if (!writeTone(lead_tone_duration)) { // remaining cycles
            printf("Failed to write lead tone of duration %f s\n", lead_tone_duration);
        }

        if (mDebugInfo.verbose) {
            if (block_no > 0)
                cout << "BLOCK " << block_no << ": LEAD TONE " << lead_tone_duration << " s : ";
            else
                cout << "BLOCK 0: PRELUDE " << prelude_lead_tone_cycles << " cycles : DUMMY BYTE : POSTLUDE " << lead_tone_duration << " s : ";
        }

        // Change lead tone duration for remaining blocks
        lead_tone_duration = other_block_lead_tone_duration;

        // --------------------- start of block header  ------------------------
        //
        // Write block header including preamble
        //
        // <preamble:1> <block name:1-10> <load adr:4> <exec adr:4> <block no:2> <block len:2> <block flag:1> <next adr:4>
        // 
        // --------------------------------------------------------------------------

        Bytes header_data;

        // Write preamble
        mCRC = 0;
        if (!writeByte(0x2a, bbmDefaultDataEncoding)) {
            cout << "Failed to write preamble 0x2a byte\n";
            return false;
        }

        // Initialise header CRC
        mCRC = 0;

        // Store header bytes
        if (!block_iter->encodeTapeHdr(header_data)) {
             cout << "Failed to encode header bytes for block #" << block_no << "\n";
            return false;
        }

        // Encode the header bytes
        BytesIter hdr_iter = header_data.begin();
        while (hdr_iter < header_data.end())
            writeByte(*hdr_iter++, bbmDefaultDataEncoding);

        //
        // Write the header CRC
        //

        // Encode CRC byte
        Word header_CRC = mCRC;

        if (!writeByte(header_CRC / 256, bbmDefaultDataEncoding) || !writeByte(header_CRC % 256, bbmDefaultDataEncoding)) {
            printf("Failed to encode header CRC!%s\n", "");
            return false;
        }


        if (mDebugInfo.verbose)
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
            writeByte(*data_iter++, bbmDefaultDataEncoding);


        //
        // Write the data CRC
        //

        Word data_CRC = mCRC;
        if (!writeByte(data_CRC / 256, atomDefaultDataEncoding) || !writeByte(data_CRC % 256, atomDefaultDataEncoding)) {
            printf("Failed to encode data CRC!%s\n", "");
            return false;
        }


        if (mDebugInfo.verbose)
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

            if (mDebugInfo.verbose)
                cout << trailer_tone_duration << " s TRAILER TONE : ";

            // Write the gap
            double block_gap = last_block_gap;
            if (mUseOriginalTiming)
                block_gap = block_iter->blockGap;

            if (!writeGap(block_gap)) {
                printf("Failed to encode a gap of %f s\n", block_gap);
                return false;
            }

            if (mDebugInfo.verbose)
                cout << block_gap << " s GAP";

        }

        if (mDebugInfo.verbose)
            cout << "\n";

        block_iter++;


        block_no++;

    }

    if (mDebugInfo.verbose)
        cout << mSamples.size() - initial_no_samples << " samples created from Tape File!\n";

    if (mSamples.size() - initial_no_samples == 0) {
        cout << "No samples could be created from Tape File!\n";

        return false;
    }



    return true;
}

/*
 * Encode Acorn Atom TAP File structure as WAV file
 */
bool WavEncoder::encodeAtom(TapeFile &tapeFile)
{
    size_t initial_no_samples = mSamples.size(); // zero unless many tape files are encoded into one and the same set of samples

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

    if (mDebugInfo.verbose)
        cout << "\nEncoding program '" << tapeFile.blocks[0].blockName() << "' as a WAV file...\n\n";
    

    int block_no = 0;
    int n_blocks = (int)tapeFile.blocks.size();

    // Encode initial gap before first block
    if (!writeGap(first_block_gap)) {
        printf("Failed to encode a gap of %f s\n", block_gap);
    }

    if (mDebugInfo.verbose)
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

        if (mDebugInfo.verbose)
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
        if (!block_iter->encodeTapeHdr(header_data)) {
            cout << "Failed to encode header for block #" << block_no << "\n";
            return false;
        }


        // Encode the header bytes
        BytesIter hdr_iter = header_data.begin();
        while (hdr_iter < header_data.end())
            writeByte(*hdr_iter++, atomDefaultDataEncoding);


        if (mDebugInfo.verbose)
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

        
        if (mDebugInfo.verbose)
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
            writeByte(*data_iter++, atomDefaultDataEncoding);




        


        //
        // Write CRC
        //

        // Encode CRC byte

        if (!writeByte(mCRC & 0xff, atomDefaultDataEncoding)) {
            printf("Failed to encode CRC!%s\n", "");
            return false;
        }

        
        if (mDebugInfo.verbose)
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

        
        if (mDebugInfo.verbose)
            cout << block_gap << " s GAP\n";
         

        block_iter++;


        block_no++;

    }

    if (mDebugInfo.verbose)
        cout << mSamples.size() << " samples created from Tape File!\n";

    if (mSamples.size() - initial_no_samples == 0) {
        cout << "No samples could be created from Tape File!\n";
        return false;
    }

	return true;
}


bool WavEncoder::writeSamples(string& filePath)
{
    if (mSamples.size() == 0)
        return false;

    // Write samples to WAV file
    Samples samples_v[] = { mSamples };
    if (!PcmFile::writeSamples(filePath, samples_v, 1, mBitTiming.fS, mDebugInfo)) {
        printf("Failed to write samples!%s\n", "");
        return false;
    }

    // Clear samples to secure that future encodings start without any initial samples
    mSamples.clear();

    return true;
}

bool WavEncoder::writeByte(Byte byte, DataEncoding encoding)
{
    if (!writeStartBit())
        return false;

    int n_data_bits = encoding.bitsPerPacket;

    Byte b = byte;
    int parity = 0;
    for (int i = 0; i < n_data_bits; i++) {
        if (!writeDataBit(b & 0x1)) {
            printf("Failed to encode '%d' data bit #%d\n", b & 0x1, i);
            return false;
        }
        b = b >> 1;
        parity += (b & 0x1) % 2;
    }

    if (encoding.parity == Parity::ODD)
        parity = 1 - parity;

    if (encoding.parity != Parity::NO_PAR && !writeDataBit(parity)) {
        cout << "Failed to write " << _PARITY(encoding.parity) << " bit\n";
        return false;
    }

    if (!writeStopBit(encoding))
        return false;

    FileBlock block = FileBlock(mTargetMachine);
    block.updateCRC(mCRC, byte);

    return true;

}


bool WavEncoder::writeDataBit(int bit)
{
    int n_cycles;
    bool high_frequency;

    if (bit != 0) {
        n_cycles = mBitTiming.highDataBitCycles;
        high_frequency = true;
    }
    else {
        n_cycles = mBitTiming.lowDataBitCycles;
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
    int n_cycles = mBitTiming.startBitCycles;

    if (!writeCycle(false, n_cycles)) {
        printf("Failed to encode start bit%s\n", "");
        return false;
    }

    return true;
}

bool WavEncoder::writeStopBit(DataEncoding encoding)
{
    int n_cycles = mBitTiming.stopBitCycles * encoding.nStopBits + (encoding.extraShortWave?1:0);

    if (!writeCycle(true, n_cycles)) {
        printf("Failed to encode stop bit%s\n", "");
        return false;
    }


    return true;
}

bool WavEncoder::writeTone(double duration)
{
    int n_cycles = (int) round((double) duration * mTapeTiming.baseFreq * 2);

    if (!writeCycle(true, n_cycles)) {
        printf("Failed to encode %lf duration (%d cycles) tone.\n", duration, n_cycles);
        return false;
    }

    /*
    if (mDebugInfo.verbose)
        printf("%lf s (%d cycles) tone written!\n", duration, n_cycles);
    */

    return true;
}

bool WavEncoder::writeGap(double duration)
{
    unsigned n_samples = (int) round(duration * mBitTiming.fS);

    if (n_samples == 0)
        return true;

    for (unsigned s = 0; s < n_samples; s++) {
        mSamples.push_back(0);
    }

    return true;
}

bool WavEncoder::writePulse(Samples& samples, Level &halfCycleLevel, int nSamples)
{
    const int maxAmplitude = 16384;
    uint16_t level = (halfCycleLevel == Level::HighLevel ? maxAmplitude : -maxAmplitude);

    for (int s = 0; s < nSamples; s++)
        samples.push_back(level);

    // Next half_cycle
    halfCycleLevel = (halfCycleLevel == Level::HighLevel ? Level::LowLevel : Level::HighLevel);

    return true;
}
bool WavEncoder::writeHalfCycle(Samples& samples, Level &halfCycleLevel, int nSamples)
{
    if (nSamples == 0)
        return false;

    const double PI = 3.14159265358979323846;
    double rad_step = PI / nSamples;
    double phase = (halfCycleLevel == Level::HighLevel ? 0.0 : PI);
    const int maxAmplitude = 16384;

    for (int s = 0; s < nSamples; s++) {
        Sample y = (Sample)round(sin(s * rad_step + phase) * maxAmplitude);
        samples.push_back(y);
    }

    // Next half_cycle
    halfCycleLevel = (halfCycleLevel == Level::HighLevel ? Level::LowLevel : Level::HighLevel);

    return true;
}

bool WavEncoder::writeCycle(bool highFreq, unsigned n)
{
    if (n == 0)
        return true;

    const double PI = 3.14159265358979323846;
    int n_samples;
    if (highFreq) {
        n_samples = (int) round(mBitTiming.F2Samples * n);

    } 
    else {
        n_samples = (int) round(mBitTiming.F1Samples * n);
    }

    double half_cycle = ((mPhase + 180) % 360) * PI / 180;

    double rad_step = 2 * n * PI / n_samples;
    for (int s = 0; s < n_samples; s++) {
        Sample y = (Sample) round(sin(s * rad_step + half_cycle) * mMaxSampleAmplitude);
        mSamples.push_back(y);
    }
    return true;
}

bool WavEncoder::setBaseFreq(double baseFreq)
{
    mTapeTiming.baseFreq = baseFreq;

    // Update bit timing as impacted by the change in Base frequency
    BitTiming new_bit_timing(mBitTiming.fS, mTapeTiming.baseFreq, mTapeTiming.baudRate, mTargetMachine);
    mBitTiming = new_bit_timing;

    return true;
}

bool WavEncoder::setBaudRate(int baudrate)
{
    mTapeTiming.baudRate = baudrate;

    // Update bit timing as impacted by the change in Baudrate
    BitTiming new_bit_timing(mBitTiming.fS, mTapeTiming.baseFreq, mTapeTiming.baudRate, mTargetMachine);
    mBitTiming = new_bit_timing;

    return true;
}

bool WavEncoder::setPhase(int phase)
{
    //mBitTiming.
    mTapeTiming.phaseShift = phase;
    return true;
}
