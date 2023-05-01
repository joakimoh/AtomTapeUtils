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


CSWCodec::CSWCodec()
{

}

CSWCodec::CSWCodec(TAPFile& tapFile, bool useOriginalTiming): mTapFile(tapFile)
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
    double block_gap = mTapeTiming.nomBlockTiming.firstBlockGap;
    double last_block_gap = mTapeTiming.nomBlockTiming.lastBlockGap;

    float high_tone_freq = mTapeTiming.baseFreq * 2;

 
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
        if (mUseOriginalTiming) {
            lead_tone_duration = (ATM_block_iter->leadToneCycles) / high_tone_freq;
            mPhase = ATM_block_iter->phaseShift;
        }
        if (!writeTone(lead_tone_duration)) {
            DBG_PRINT(ERR, "Failed to write lead tone of duration %f s\n", lead_tone_duration);
        }
        DBG_PRINT(DBG, "Lead tone written%s\n", "");

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

        DBG_PRINT(DBG, "Block header written%s\n", "");


        // --------------------------------------------------------------------------
        //
        // ---------------- End of block header chunk -------------------------------


        // Add micro lead/trailer tone between header and data
        if (mUseOriginalTiming)
            data_block_micro_lead_tone_duration = (ATM_block_iter->microToneCycles) / high_tone_freq;
        if (!writeTone(data_block_micro_lead_tone_duration)) {
            DBG_PRINT(ERR, "Failed to write micro lead tone of duration %f s\n", data_block_micro_lead_tone_duration)
        }

        DBG_PRINT(DBG, "Micro lead tone written%s\n", "");


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

        DBG_PRINT(DBG, "Block data written%s\n", "");



        //
        // Write CRC
        //

        // Encode CRC byte

        if (!writeByte(mCRC)) {
            DBG_PRINT(ERR, "Failed to encode CRC!%s\n", "");
            return false;
        }

        DBG_PRINT(DBG, "CRC written%s\n", "");

        ATM_block_iter++;

        // --------------------------------------------------------------------------
        //
        // ---------------- End of block data chunk -------------------------------

        //
        // No trailer tone added as that hasn't been observed to exist!
        //




        // Write a gap at the end of the block
        if (mUseOriginalTiming)
            block_gap = (ATM_block_iter->blockGap);
        else if (block_no == n_blocks - 1)
            block_gap = last_block_gap;

        if (!writeGap(block_gap)) {
            DBG_PRINT(ERR, "Failed to encode a gap of %f s\n", block_gap);
        }

        DBG_PRINT(DBG, "Block gap written%s\n", "");


        DBG_PRINT(DBG, "Block #%d written\n", block_no);


        block_no++;

    }


    // Write samples to CSW file
    CSW2Hdr hdr;
    ofstream fout(filePath, ios::out | ios::binary | ios::ate);

    // Write CSW header
    hdr.csw2.compType = 0x02; // (Z - RLE) - ZLIB compression of data stream
    hdr.csw2.flags = 0;
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
    writeCompressedPulses(filePath, fout, (streamsize) mPulses.size(), mPulses);

    DBG_PRINT(DBG, "CSW file completed!%s\n", "");

    // Clear samples to secure that future encodings start without any initial samples
    mPulses.clear();

    return true;

}

bool CSWCodec::decode(string &CSWFileName, Bytes& pulses, int& sampleFreq, CycleDecoder::Phase& firstPhase)
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
        firstPhase = ((csw2_hdr.flags & 0x01) == 0x01 ? CycleDecoder::High : CycleDecoder::Low);
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
        firstPhase = ((csw1_hdr.flags & 0x01) == 0x01 ? CycleDecoder::High : CycleDecoder::Low);
    }



    // Assign intial level to pulse (High or Low)
    mPulseLevel = firstPhase;


    // Get size of pulse data
    streamsize data_sz = file_size - fin.tellg();

    cout << "CSW v" << (int)common_hdr.majorVersion << "." << (int)common_hdr.minorVersion << " format with settings:\n";
    cout << "compressed: " << (compressed ? "Z-RLE" : "RLE") << "\n";
    cout << "sample frequency: " << (int)sampleFreq << "\n";
    cout << "no of pulses: " << (int)n_pulses << "\n";
    cout << "initial polarity: " << (firstPhase == CycleDecoder::High ? "High" : "Low") << "\n";
    cout << "encoding app: " << encoding_app << "\n";
    cout << "data size of pulses: " << data_sz << "\n";


    //
    // Now read data (i.e. the pulses)
    //


    if (compressed) { // Data is  compressed with ZLIB - need to uncompress it first
        if (!readCompressedPulses(CSWFileName, fin, n_pulses, pulses)) {
            fin.close();
            return false;
        }
    } else { // Data is not compressed with ZLIB - just read it

        // Collect all pulses into a vector 'pulses'
        pulses.resize((int)data_sz);
        fin.read((char*)&pulses.front(), data_sz);
        fin.close();
    }

    DBG_PRINT(DBG, "CSW File '%s' data decoded into %d pulses of sample frequency %d!\n", CSWFileName.c_str(), sampleFreq);
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
        DBG_PRINT(ERR, "Failed to encode '%d' data bit #%d\n", high_frequency, bit);
        return false;
    }


    return true;
}

bool CSWCodec::writeStartBit()
{
    int n_cycles = mStartBitCycles;

    if (!writeCycle(false, n_cycles)) {
        DBG_PRINT(ERR, "Failed to encode start bit%s\n", "");
        return false;
    }

    return true;
}

bool CSWCodec::writeStopBit()
{
    int n_cycles = mStopBitCycles;

    if (!writeCycle(true, n_cycles)) {
        DBG_PRINT(ERR, "Failed to encode stop bit%s\n", "");
        return false;
    }


    return true;
}

bool CSWCodec::writeTone(double duration)
{
    int n_cycles = (int)round((double)duration * F2_FREQ);

    if (!writeCycle(true, n_cycles)) {
        DBG_PRINT(ERR, "Failed to encode %lf duration (%d cycles) tone.\n", duration, n_cycles);
        return false;
    }

    DBG_PRINT(DBG, "%lf s (%d cycles) tone written!\n", duration, n_cycles);

    return true;
}

// Gap is coded as a long low pulse
//

bool CSWCodec::writeGap(double duration)
{
    int n_samples = (int)round(duration * mFS);

    // Insert one very short high pulse if not already an high pulse
    if (mPulseLevel == CycleDecoder::Phase::Low)
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

    mPulseLevel = CycleDecoder::Phase::Low;

    return true;
}

bool CSWCodec::writeCycle(bool highFreq, int n)
{
    const double PI = 3.14159265358979323846;
    int n_samples;
    if (highFreq) {
        n_samples = (int)round(mHighSamples * n);
        DBG_PRINT(DBG, "%d cycles of F2\n", n);
    }
    else {
        DBG_PRINT(DBG, "%d cycles of F1\n", n);
        n_samples = (int)round(mLowSamples * n);
    }

    double phase = ((mPhase + 180) % 360) * PI / 180;

    double f = 2 * n * PI / n_samples;
    int s = 0;
    int ps = s;
    while (s < n_samples) {
        if (sin(s * f + phase) < 0 && mPulseLevel == CycleDecoder::Phase::High && s - ps > 0) {
            mPulses.push_back(s - ps);
            ps = s;
            mPulseLevel = CycleDecoder::Phase::Low;
        } else if (sin(s* f + phase) > 0 && mPulseLevel == CycleDecoder::Phase::Low && s - ps > 0) {
            mPulses.push_back(s - ps);
            ps = s;
            mPulseLevel = CycleDecoder::Phase::High;
        }
        s++;
    }
    return true;
}


// Read compressed pulses
bool CSWCodec::readCompressedPulses(string fin_name, ifstream& fin, int n_pulses, Bytes& pulses)
{

    // Create temp file name
    string src_tmp_file_name, dst_tmp_file_name;
    crTmpFile(fin_name, src_tmp_file_name, dst_tmp_file_name);

    // Create and open the src temp file
    ofstream f_src = ofstream(src_tmp_file_name, ios::out | ios::binary | ios::ate);

    if (!f_src) {
        cout << "Failed to create temp src file\n";
        return false;
    }

    // Copy pulse data to the src temp file
    int buf_sz = 1 << 20;
    char* buf = new char[buf_sz];
    streamsize n_written_bytes = 0;
    while (fin)
    {
        fin.read(buf, buf_sz);
        size_t n = fin.gcount();
        if (!n)
            break;
        n_written_bytes += n;
        f_src.write(buf, buf_sz);
    }
    delete[] buf; // free buffer used when reading temp file
    
    fin.close();
    f_src.close();
    cout << "Wrote " << n_written_bytes << " bytes to file\n";

    // Decompress the pulse data stored in the src_tmp_file_name into dst_tmp_file_name
    if (!inflate(src_tmp_file_name, dst_tmp_file_name)) {
        cout << "Failed to decompress pulse data\n";
        return false;
    }

    // Open the dst temp file with the uncompressed pulses
    ifstream f_dst(dst_tmp_file_name.c_str(), ios::in | ios::binary | ios::ate);

    if (!f_dst) {
        DBG_PRINT(ERR, "Failed to open file '%s'\n", dst_tmp_file_name.c_str());
        return false;
    }

    // Collect all pulses into a vector 'pulses' 
    pulses.resize(n_pulses);
    char* pulses_p = (char*)&pulses.front();
    f_dst.seekg(0);
    if (!f_dst.read(pulses_p, (streamsize) n_pulses)) {
        cout << "Less pulses (" << f_dst.gcount() << ") then expected (" << n_pulses << ") after decompression!\n";
    }

    f_dst.close();
    if (remove(src_tmp_file_name.c_str()) != 0) {
         cout << "Failed to remove temp file " << src_tmp_file_name << "\n";
    }

    if (remove(dst_tmp_file_name.c_str()) != 0) {
        cout << "Failed to remove temp file " << dst_tmp_file_name << "\n";
    }

    return true;
}

// Write compressed pulses
bool CSWCodec::writeCompressedPulses(string fout_name, ofstream& fout, streamsize data_sz, Bytes& pulses)
{
    // Create temp file name
    string src_tmp_file_name, dst_tmp_file_name;
    crTmpFile(fout_name, src_tmp_file_name, dst_tmp_file_name);

    // Write the uncompressed pulses to the src temp file
    ofstream f_src(src_tmp_file_name, ios::out | ios::binary | ios::ate);
    if (!f_src.write((char*)&pulses.front(), data_sz)) {
        cout << "Failed to write uncompressed pulses to temp file!\n";
        return false;
    }
    f_src.close();

    // Store compressed pulses in dst temp file
    if (!deflate(src_tmp_file_name, dst_tmp_file_name)) {
        cout << "Failed to compress the pulses!\n";
        return false;
    }

    // Open the dst file with the compressed pulses and transfer the compressed data to the CSW file
    ifstream f_dst(dst_tmp_file_name, ios::in | ios::binary | ios::ate);
    f_dst.seekg(0);
    int buf_sz = 1 << 20;
    char* buf = new char[buf_sz];
    while (f_dst)
    {
        f_dst.read(buf, buf_sz);
        size_t n = f_dst.gcount();
        if (!n)
            break;
        fout.write(buf, buf_sz);
    }
    delete[] buf; // free buffer used when reading temp file
    //remove(tmp_file_name.c_str()); // remove temp file
    f_dst.close();
    fout.close();

    return true;
}

bool CSWCodec::crTmpFile(string file_name, string &src_tmp_file_name, string &dst_tmp_file_name)
{
    filesystem::path fin_p = file_name;
    string fin_base_name = fin_p.stem().string();
    src_tmp_file_name = fin_base_name + "_src.tmp";
    dst_tmp_file_name = fin_base_name + "_dst.tmp";

    cout << "Source Temp file is '" << src_tmp_file_name << "'\n";
    cout << "Destination Temp file is '" << dst_tmp_file_name << "'\n";


    return true;
}