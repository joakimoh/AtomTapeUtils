#include <fstream>
#include <filesystem>
#include <iostream>
#include "CommonTypes.h"
#include "WavEncoder.h"
#include "../shared/WaveSampleTypes.h"
#include "../shared/PcmFile.h"
#include "../shared/Debug.h"
#include "../shared/Utility.h"

using namespace std;


WavEncoder::WavEncoder(TAPFile& tapFile, int baudrate): mTapFile(tapFile), mBaudrate(baudrate)
{
    if (mBaudrate == 300) {
        mStartBitCycles = 4; // Start bit length in cycles of F1 frequency carrier
        mLowDataBitCycles = 4; // Data bit length in cycles of F1 frequency carrier
        mHighDataBitCycles = 8; // Data bit length in cycles of F2 frequency carrier
        mStopBitCycles = 9; // Stop bit length in cycles of F2 frequency carrier
    }
    else if (mBaudrate == 1200) {
        mStartBitCycles = 1; // Start bit length in cycles of F1 frequency carrier
        mLowDataBitCycles = 1; // Data bit length in cycles of F1 frequency carrier
        mHighDataBitCycles = 2; // Data bit length in cycles of F2 frequency carrier
        mStopBitCycles = 3; // Stop bit length in cycles of F2 frequency carrier
    }
    else {
        throw invalid_argument("Unsupported baud rate " + baudrate);
    }
}

WavEncoder::WavEncoder(int baudrate)
{
    if (mBaudrate == 300) {
        mStartBitCycles = 4; // Start bit length in cycles of F1 frequency carrier
        mLowDataBitCycles = 4; // Data bit length in cycles of F1 frequency carrier
        mHighDataBitCycles = 8; // Data bit length in cycles of F2 frequency carrier
        mStopBitCycles = 9; // Stop bit length in cycles of F2 frequency carrier
    }
    else if (mBaudrate == 1200) {
        mStartBitCycles = 1; // Start bit length in cycles of F1 frequency carrier
        mLowDataBitCycles = 1; // Data bit length in cycles of F1 frequency carrier
        mHighDataBitCycles = 2; // Data bit length in cycles of F2 frequency carrier
        mStopBitCycles = 3; // Stop bit length in cycles of F2 frequency carrier
    }
    else {
        throw invalid_argument("Unsupported baud rate " + baudrate);
    }
}

/*
 * Encode TAP File structure as WAV file
 */
bool WavEncoder::encode(string& filePath)
{
    double lead_tone_duration = mLeadToneDuration;
    double other_block_lead_tone_duration = mOtherBlockLeadToneDuration;
    double data_block_micro_lead_tone_duration = mDataBlockMicroLeadToneDuration;
    double block_gap = mBlockGap;
    double last_block_gap = mLastBlock_gap;

   

    if (mTapFile.blocks.empty())
        return false;


    ATMBlockIter ATM_block_iter = mTapFile.blocks.begin();

    DBG_PRINT(DBG, "#blocks = %d\n", (int)mTapFile.blocks.size());

    

    int block_no = 0;
    int n_blocks = (int)mTapFile.blocks.size();

    // Encode initial gap before first block
    if (!writeGap(block_gap)) {
        DBG_PRINT(ERR, "Failed to encode a gap of %f s\n", block_gap);
    }

    while (ATM_block_iter < mTapFile.blocks.end()) {


        // Write a lead tone for the block
        if (!writeTone(lead_tone_duration)) {
            DBG_PRINT(ERR, "Failed to write lead tone of duration %f s\n", lead_tone_duration);
        }
        DBG_PRINT(DBG, "Lead tone written\n");

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


        int data_len = ATM_block_iter->hdr.lenHigh * 256 + ATM_block_iter->hdr.lenLow;  // get data length

        Byte b7 = (block_no < n_blocks - 1 ? 0x80 : 0x00);          // calculate flags
        Byte b6 = (data_len > 0 ? 0x40 : 0x00);
        Byte b5 = (block_no != 0 ? 0x20 : 0x00);

        Bytes header_data;

        // store preamble
        for (int i = 0; i < 4; i++)
            header_data.push_back(0x2a);

        // store block name
        int name_len = 0;
        for (; name_len < sizeof(ATM_block_iter->hdr.name) && ATM_block_iter->hdr.name[name_len] != 0; name_len++);
        for (int i = 0; i < name_len; i++)
            header_data.push_back(ATM_block_iter->hdr.name[i]);

        header_data.push_back(0xd);

        header_data.push_back(b7 | b6 | b5);                        // store flags

        header_data.push_back((block_no >> 8) & 0xff);              // store block no
        header_data.push_back(block_no & 0xff);

        header_data.push_back((data_len > 0 ? data_len - 1 : 0));   // store length - 1

        header_data.push_back(ATM_block_iter->hdr.execAdrHigh);     // store execution address
        header_data.push_back(ATM_block_iter->hdr.execAdrLow);

        header_data.push_back(ATM_block_iter->hdr.loadAdrHigh);     // store load address
        header_data.push_back(ATM_block_iter->hdr.loadAdrLow);


        // Encode the header bytes
        BytesIter hdr_iter = header_data.begin();
        while (hdr_iter < header_data.end())
            writeByte(*hdr_iter++);

        DBG_PRINT(DBG, "Block header written\n");
     

        // --------------------------------------------------------------------------
        //
        // ---------------- End of block header chunk -------------------------------


        // Add micro lead/trailer tone between header and data
        if (!writeTone(data_block_micro_lead_tone_duration)) {
            DBG_PRINT(ERR, "Failed to write micro lead tone of duration %f s\n", data_block_micro_lead_tone_duration)
        }

        DBG_PRINT(DBG, "Micro lead tone written\n");


        // --------------------- start of block data + CRC chunk ------------------------
         //
         // Write block data as one chunk
         //
         // --------------------------------------------------------------------------

        // Write block data
        BytesIter bi = ATM_block_iter->data.begin();
        Bytes block_data;
        while (bi < ATM_block_iter->data.end())
            block_data.push_back(*bi++);

        // Encode the block data bytes
        BytesIter data_iter = block_data.begin();
        while (data_iter < block_data.end())
            writeByte(*data_iter++);

        DBG_PRINT(DBG, "Block data written\n");
        


        //
        // Write CRC
        //

        // Encode CRC byte

        if (!writeByte(mCRC)) {
            DBG_PRINT(ERR, "Failed to encode CRC\n");
            return false;
        }

        DBG_PRINT(DBG, "CRC written\n");

        ATM_block_iter++;

        // --------------------------------------------------------------------------
        //
        // ---------------- End of block data chunk -------------------------------

        //
        // No trailer tone added as that hasn't been observed to exist!
        //




        // Write a gap at the end of the block
        if (block_no == n_blocks - 1)
            block_gap = last_block_gap;

        if (!writeGap(block_gap)) {
            DBG_PRINT(ERR, "Failed to encode a gap of %f s\n", block_gap);
        }

        DBG_PRINT(DBG, "Block gap written\n");


        DBG_PRINT(DBG, "Block #%d written\n", block_no);


        block_no++;

    }



    // Write samples to WAV file
    Samples samples_v[] = { mSamples };
    if (!writeSamples(filePath, samples_v, 1)) {
        DBG_PRINT(ERR, "Failed to write samples!\n");
        return false;
    }


    DBG_PRINT(DBG, "WAV file completed!\n");

    // Clear samples to secure that future encodings start without any initial samples
    mSamples.clear();

	return true;
}


/*
 * Get the encoder's TAP file
 */
bool WavEncoder::getTAPFile(TAPFile& tapFile)
{
	tapFile = mTapFile;

	return true;
}

/*
 * Reinitialise encoder with a new TAP file
 */
bool WavEncoder::setTAPFile(TAPFile& tapFile)
{
	mTapFile = tapFile;

	return true;
}

bool WavEncoder::writeByte(Byte byte)
{
    if (!writeStartBit())
        return false;

    Byte b = byte;
    for (int i = 0; i < 8; i++) {
        DBG_PRINT(DBG, "Write bit %d = '%d' for byte %.2x\n", i, b & 0x1, byte);
        if (!writeDataBit(b & 0x1)) {
            DBG_PRINT(ERR, "Failed to encode '%d' data bit #%d\n", b & 0x1, i);
            return false;
        }
        b = b >> 1;
    }

    if (!writeStopBit())
        return false;

    mCRC += byte;

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
        DBG_PRINT(ERR, "Failed to encode '%d' data bit #%d\n", high_frequency, bit);
        return false;
    }


    return true;
}

bool WavEncoder::writeStartBit()
{
    int n_cycles = mStartBitCycles;

    if (!writeCycle(false, n_cycles)) {
        DBG_PRINT(ERR, "Failed to encode start bit\n");
        return false;
    }

    return true;
}

bool WavEncoder::writeStopBit()
{
    int n_cycles = mStopBitCycles;

    if (!writeCycle(true, n_cycles)) {
        DBG_PRINT(ERR, "Failed to encode stop bit\n");
        return false;
    }


    return true;
}

bool WavEncoder::writeTone(double duration)
{
    int n_cycles = (int) round((double) duration * F2_FREQ);

    if (!writeCycle(true, n_cycles)) {
        DBG_PRINT(ERR, "Failed to encode %lf duration (%d cycles) tone.\n", duration, n_cycles);
        return false;
    }

    DBG_PRINT(DBG, "%lf s (%d cycles) tone written!\n", duration, n_cycles);

    return true;
}

bool WavEncoder::writeGap(double duration)
{
    int n_samples = (int) round(duration * F_S);
    for (int s = 0; s < n_samples; s++) {
        mSamples.push_back(0);
    }

    return true;
}

bool WavEncoder::writeCycle(bool highFreq, int n)
{
    const double PI = 3.14159265358979323846;
    int n_samples;
    if (highFreq) {
        n_samples = (int) round(mHighSamples * n);
        DBG_PRINT(DBG, "%d cycles of F2\n", n);
    } 
    else {
        DBG_PRINT(DBG, "%d cycles of F1\n", n);
        n_samples = (int) round(mLowSamples * n);
    }

    double f = 2 * n * PI / n_samples;
    for (int s = 0; s < n_samples; s++) {
        Sample y = (Sample) round(sin(s * f) * mMaxSampleAmplitude);
        mSamples.push_back(y);
    }
    return true;
}