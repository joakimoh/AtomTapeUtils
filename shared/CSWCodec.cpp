#include <gzstream.h>
#include <stdlib.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string.h>
#include "CSWCodec.h"
#include "CommonTypes.h"
#include "BlockTypes.h"
#include "Debug.h"
#include "Utility.h"
#include "BlockTypes.h"
#include "WaveSampleTypes.h"
#include "zpipe.h"



using namespace std;


CSWCodec::CSWCodec(bool verbose) : mVerbose(verbose)
{

}

CSWCodec::CSWCodec(TAPFile& tapFile, bool useOriginalTiming, bool verbose): mTapFile(tapFile), mVerbose(verbose)
{
    mUseOriginalTiming = useOriginalTiming;
}


bool CSWCodec::setTapeTiming(TapeProperties tapeTiming)
{
    mTapeTiming = tapeTiming;
    return true;
}

bool CSWCodec::getTAPFile(TAPFile& tapFile)
{
    tapFile = mTapFile;

    return true;
}

bool CSWCodec::setTAPFile(TAPFile& tapFile)
{
    mTapFile = tapFile;

    return true;
}


bool CSWCodec::encode(string &filePath, int sampleFreq)
{

    mFS = sampleFreq;
    if (mTapeTiming.baudRate == 300) {
        mStartBitCycles = 4; // Start bit length in cycles of F1 frequency carrier
        mLowDataBitCycles = 4; // Data bit length in cycles of F1 frequency carrier
        mHighDataBitCycles = 8; // Data bit length in cycles of F2 frequency carrier
        mStopBitCycles = 9; // Stop bit length in cycles of F2 frequency carrier
    }
    else if (mTapeTiming.baudRate == 1200) {
        mStartBitCycles = 1; // Start bit length in cycles of F1 frequency carrier
        mLowDataBitCycles = 1; // Data bit length in cycles of F1 frequency carrier
        mHighDataBitCycles = 2; // Data bit length in cycles of F2 frequency carrier
        mStopBitCycles = 3; // Stop bit length in cycles of F2 frequency carrier
    }
    else {
        throw invalid_argument("Unsupported baud rate " + mTapeTiming.baudRate);
    }
    mHighSamples = (double)mFS / F2_FREQ;
    mLowSamples = (double)mFS / F1_FREQ;

    double lead_tone_duration = mTapeTiming.nomBlockTiming.firstBlockLeadToneDuration;
    double other_block_lead_tone_duration = mTapeTiming.nomBlockTiming.otherBlockLeadToneDuration;
    double data_block_micro_lead_tone_duration = mTapeTiming.nomBlockTiming.microLeadToneDuration;
    double first_block_gap = mTapeTiming.nomBlockTiming.firstBlockGap;
    double block_gap = mTapeTiming.nomBlockTiming.blockGap;
    double last_block_gap = mTapeTiming.nomBlockTiming.lastBlockGap;
    

    float high_tone_freq = mTapeTiming.baseFreq * 2;

 
    if (mTapFile.blocks.empty())
        return false;


    if (mVerbose)
        cout << "\nEncode program '" << mTapFile.blocks[0].hdr.name << "' as a CSW file...\n\n";


    ATMBlockIter ATM_block_iter = mTapFile.blocks.begin();




    int block_no = 0;
    int n_blocks = (int)mTapFile.blocks.size();

    // Encode initial gap before first block
    if (!writeGap(first_block_gap)) {
        printf("Failed to encode a gap of %f s\n", block_gap);
    }

    if (mVerbose)
        cout << first_block_gap << " s GAP\n";


    while (ATM_block_iter < mTapFile.blocks.end()) {

        // Write a lead tone for the block
        if (mUseOriginalTiming) {
            lead_tone_duration = (ATM_block_iter->leadToneCycles) / high_tone_freq;
            mPhase = ATM_block_iter->phaseShift;
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

        if (mVerbose)
            cout << "HDR : ";

        // --------------------------------------------------------------------------
        //
        // ---------------- End of block header chunk -------------------------------


        // Add micro tone between header and data
        if (mUseOriginalTiming)
            data_block_micro_lead_tone_duration = (ATM_block_iter->microToneCycles) / high_tone_freq;
        if (!writeTone(data_block_micro_lead_tone_duration)) {
            printf("Failed to write micro lead tone of duration %f s\n", data_block_micro_lead_tone_duration);
        }

        if (mVerbose)
            cout << data_block_micro_lead_tone_duration << " s micro tone : ";


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

        //
        // Write CRC
        //

        // Encode CRC byte

        if (!writeByte(mCRC)) {
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
            block_gap = (ATM_block_iter->blockGap);
        else if (block_no == n_blocks - 1)
            block_gap = last_block_gap;

        

        if (!writeGap(block_gap)) {
            printf("Failed to encode a gap of %f s\n", block_gap);
        }

        if (mVerbose)
            cout << block_gap << " s GAP\n";

        ATM_block_iter++;

        block_no++;


    }



    // Write samples to CSW file
    CSW2Hdr hdr;
    ofstream fout(filePath, ios::out | ios::binary | ios::ate);

    // Write CSW header
    hdr.csw2.compType = 0x02; // (Z - RLE) - ZLIB compression of data stream
    hdr.csw2.flags = 0; // Initial polarity (specified by bit b0) is LOW
    hdr.csw2.hdrExtLen = 0;
    hdr.csw2.sampleRate[0] = mFS & 0xff;
    hdr.csw2.sampleRate[1] = (mFS >> 8) & 0xff;
    hdr.csw2.sampleRate[2] = (mFS >> 16) & 0xff;
    hdr.csw2.sampleRate[3] = (mFS >> 24) & 0xff;
    int n_pulses = mPulses.size();
    hdr.csw2.totNoPulses[0] = n_pulses & 0xff;
    hdr.csw2.totNoPulses[1] = (n_pulses >> 8) & 0xff;
    hdr.csw2.totNoPulses[2] = (n_pulses >> 16) & 0xff;
    hdr.csw2.totNoPulses[3] = (n_pulses >> 24) & 0xff;
    fout.write((char*)&hdr, sizeof(hdr));


    // Write compressed samples
    if (!encodeBytes(mPulses, fout)) {
        cout << "Failed to compress CSW pulses and write them to file '" << filePath << "'!\n";
        fout.close();
        return false;
    }
    // Add one dummy byte to the end as e.g. CSW viewer seems to read one byte extra (which it shouldn't!)
    char dummy_bytes[] = "0";
    fout.write((char*)&dummy_bytes[0], sizeof(dummy_bytes));

    fout.close();
 
    // Clear samples to secure that future encodings start without any initial samples
    mPulses.clear();

    if (mVerbose)
        cout << "\nDone encoding program '" << mTapFile.blocks[0].hdr.name << "' as a CSW file...\n\n";

    return true;

}

bool CSWCodec::decode(string &CSWFileName, Bytes& pulses, int& sampleFreq, HalfCycle& firstHalfCycle)
{ 
    ifstream fin(CSWFileName, ios::in | ios::binary | ios::ate);

    if (!fin) {
        cout << "Failed to open file '" << CSWFileName << "'\n";
        return false;
    }


    // Get file size
    fin.seekg(0, ios::end);
    streamsize file_size = fin.tellg();
 
    // Repositon to start of file
    fin.seekg(0);

    // Read version-independent part of CSW header
    CSWCommonHdr common_hdr;
    if (
        !fin.read((char*)&common_hdr, sizeof(common_hdr)) || 
        common_hdr.terminatorCode != 0x1a ||
        strncmp(common_hdr.signature, "Compressed Square Wave", 22) != 0
        ) {
        cout << "File " << CSWFileName << " is not a valid CSW file (couldn't read a complete header)!\n";
        return false;
    }

    if (!(
        (common_hdr.majorVersion == 0x02 && common_hdr.minorVersion <= 0x01) ||
        (common_hdr.majorVersion == 0x01 && common_hdr.minorVersion == 0x01)
        )) {
        cout << "File " << CSWFileName << " has unsupported format v" << (int) common_hdr.majorVersion << "." << (int) common_hdr.minorVersion << "!\n";
        return false;
    }

    bool compressed;
    int n_pulses;
    string encoding_app = "???";
    if (common_hdr.majorVersion == 0x02) {
        CSW2MainHdr csw2_hdr;
        if (!fin.read((char*)&csw2_hdr, sizeof(csw2_hdr))) {
            cout << "Failed to read complete CSW header of file '" << CSWFileName << "'!\n";
            return false;
        }
        compressed = (csw2_hdr.compType == 0x02);
        sampleFreq = csw2_hdr.sampleRate[0] + (csw2_hdr.sampleRate[1] << 8) + (csw2_hdr.sampleRate[2] << 16) + (csw2_hdr.sampleRate[3] << 24);
        n_pulses = csw2_hdr.totNoPulses[0] + (csw2_hdr.totNoPulses[1] << 8) + (csw2_hdr.totNoPulses[2] << 16) + (csw2_hdr.totNoPulses[3] << 24);
        firstHalfCycle = ((csw2_hdr.flags & 0x01) == 0x01 ? HighHalfCycle : LowHalfCycle);
        char s[16];
        strncpy(s, csw2_hdr.encodingApp, 16);
        encoding_app = s;

        // Skip extension part
        if (csw2_hdr.hdrExtLen > 0)
            fin.ignore(csw2_hdr.hdrExtLen);

     }
    else {
        CSW1MainHdr csw1_hdr;
        if (!fin.read((char*)&csw1_hdr, sizeof(csw1_hdr))) {
            cout << "Failed to read complete CSW header of file '" << CSWFileName << "'!\n";
            return false;
        }
        compressed = false;
        sampleFreq = csw1_hdr.sampleRate[0] + (csw1_hdr.sampleRate[1] << 8);
        n_pulses = -1; // unspecified and therefore undefined for CSW format 1.1
        firstHalfCycle = ((csw1_hdr.flags & 0x01) == 0x01 ? HighHalfCycle : LowHalfCycle);
    }



    // Assign intial level to pulse (High or Low)
    mPulseLevel = firstHalfCycle;


    // Get size of pulse data
    streamsize data_sz = file_size - fin.tellg();

    if (mVerbose) {
        cout << "CSW v" << (int)common_hdr.majorVersion << "." << (int)common_hdr.minorVersion << " format with settings:\n";
        cout << "compressed: " << (compressed ? "Z-RLE" : "RLE") << "\n";
        cout << "sample frequency: " << (int)sampleFreq << "\n";
        cout << "no of pulses: " << (int)n_pulses << "\n";
        cout << "initial polarity: " << _HALF_CYCLE(firstHalfCycle) << "\n";
        cout << "encoding app: " << encoding_app << "\n";
    }
 

    //
    // Now read data (i.e. the pulses)
    //


    if (compressed) { // Data is  compressed with ZLIB - need to uncompress it first
        pulses.resize((int)n_pulses);
        if (!decodeBytes(fin, pulses)) {
            cout << "Failed to uncompress CSW data stored in file '" << CSWFileName << "'!\n";
            fin.close();
            return false;
        }
        fin.close();
    } else { // Data is not compressed with ZLIB - just read it

        // Collect all pulses into a vector 'pulses'
        pulses.resize((int)data_sz);
        fin.read((char*)&pulses.front(), data_sz);
        fin.close();
    }

    if (mVerbose)
        cout << "CSW File '" << CSWFileName << " - no of resulting pulses after decompression = " << pulses.size() << "\n";

    return true;
}

// Tell whether a file is a CSW
bool CSWCodec::isCSWFile(string& CSWFileName)
{
    ifstream fin(CSWFileName, ios::in | ios::binary | ios::ate);

    if (!fin) {
        cout << "Failed to open file '" << CSWFileName << "'\n";
        return false;
    }

    // Reposition to start of file
    fin.seekg(0);

    // Read CSW header
    CSWCommonHdr hdr;
    if (!fin.read((char*)&hdr, (streamsize) sizeof(hdr))) {
        return false;
    }


    if (
        hdr.terminatorCode != 0x1a ||
        strncmp(hdr.signature, "Compressed Square Wave", 22) != 0
        ) {
        return false;
    }


    return true;
}

bool CSWCodec::writeByte(Byte byte)
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

    mCRC += byte;

    return true;

}

bool CSWCodec::writeDataBit(int bit)
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

bool CSWCodec::writeStartBit()
{
    int n_cycles = mStartBitCycles;

    if (!writeCycle(false, n_cycles)) {
        printf("Failed to encode start bit%s\n", "");
        return false;
    }

    return true;
}

bool CSWCodec::writeStopBit()
{
    int n_cycles = mStopBitCycles;

    if (!writeCycle(true, n_cycles)) {
        printf("Failed to encode stop bit%s\n", "");
        return false;
    }


    return true;
}

bool CSWCodec::writeTone(double duration)
{
    
    int n_cycles = (int)round((double)duration * F2_FREQ);

    if (!writeCycle(true, n_cycles)) {
        printf("Failed to encode %lf duration (%d cycles) tone.\n", duration, n_cycles);
        return false;
    }


    return true;
}

// Gap is coded as a long low pulse
//
bool CSWCodec::writeGap(double duration)
{

    unsigned n_samples = (unsigned) round(duration * mFS);

    if (n_samples == 0)
        return true;

    // Insert one very short high pulse if not already an high pulse
    if (mPulseLevel == HalfCycle::LowHalfCycle)
        mPulses.push_back(1);

    // Insert the long low pulse
    if (n_samples <= 255)
        // Gap can be represented by a single pulse
        mPulses.push_back(n_samples);
    else {
        // Gap needs to be represented by an extended pulse
        // A zero-length pulse followed by a 4-bytes duration
        mPulses.push_back(0);
        mPulses.push_back(n_samples % 256);
        mPulses.push_back((n_samples >> 8) % 256);
        mPulses.push_back((n_samples >> 16) % 256);
        mPulses.push_back((n_samples >> 24) % 256);
    }

    mPulseLevel = HalfCycle::LowHalfCycle;

    return true;
}

bool CSWCodec::writeCycle(bool highFreq, unsigned n)
{
    if (n == 0)
        return true;

    double n_samples_per_half_cycle;
    if (highFreq) {
        n_samples_per_half_cycle = mHighSamples / 2;
    }
    else {
        n_samples_per_half_cycle = mLowSamples / 2;       
    }

    double sample_no = 0;
    double prev_sample_no = - n_samples_per_half_cycle;
    for (int c = 0; c < n; c++) {

        //  first half cycle
        int n_samples = (int) round(sample_no)  - (int) round(prev_sample_no);
         mPulses.push_back(n_samples);
 
        // Advance sample index to next 1/2 cycle
        prev_sample_no = sample_no;
        sample_no += n_samples_per_half_cycle;
 
        // Write second half cycle
        n_samples = (int)round(sample_no) - (int)round(prev_sample_no);
        mPulses.push_back(n_samples);
     
        // Advance sample index to next cycle
        prev_sample_no = sample_no;
        sample_no += n_samples_per_half_cycle;

    }
 
    return true;
}