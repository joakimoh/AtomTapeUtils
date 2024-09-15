#include <gzstream.h>
#include <stdlib.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string.h>
#include "CSWCodec.h"
#include "CommonTypes.h"
#include "AtomBlockTypes.h"
#include "Debug.h"
#include "Utility.h"
#include "AtomBlockTypes.h"
#include "WaveSampleTypes.h"
#include "zpipe.h"
#include "FileBlock.h"



using namespace std;


CSWCodec::CSWCodec(int sampleFreq, TapeProperties tapeTiming, bool verbose, TargetMachine targetMachine) : 
    mBitTiming(sampleFreq, tapeTiming.baseFreq, tapeTiming.baudRate, targetMachine), mVerbose(verbose), mTargetMachine(targetMachine)
{
    if (!targetMachine)
        mTapeTiming = atomTiming;
    else if (targetMachine == ACORN_ATOM)
        mTapeTiming = atomTiming;
    else
        mTapeTiming = defaultTiming;
}

CSWCodec::CSWCodec(bool useOriginalTiming, int sampleFreq, TapeProperties tapeTiming, bool verbose, TargetMachine targetMachine):
    mBitTiming(sampleFreq, tapeTiming.baseFreq, tapeTiming.baudRate, targetMachine), mVerbose(verbose), mTargetMachine(targetMachine)
{
    mUseOriginalTiming = useOriginalTiming;
    if (!targetMachine)
        mTapeTiming = atomTiming;
    else if (targetMachine == ACORN_ATOM)
        mTapeTiming = atomTiming;
    else
        mTapeTiming = defaultTiming;
}


bool CSWCodec::encode(TapeFile& tapeFile, string& filePath)
{
    if (tapeFile.fileType <= BBC_MASTER)
        return encodeBBM(tapeFile, filePath);
    else
        return encodeAtom(tapeFile, filePath);
}

bool CSWCodec::encodeBBM(TapeFile& tapeFile, string& filePath)
{
    TargetMachine file_block_type = BBC_MODEL_B;


    int prelude_tone_cycles = mTapeTiming.nomBlockTiming.firstBlockPreludeLeadToneCycles;
    double lead_tone_duration = mTapeTiming.nomBlockTiming.firstBlockLeadToneDuration;
    double other_block_lead_tone_duration = mTapeTiming.nomBlockTiming.otherBlockLeadToneDuration;
    double trailer_tone_duration = mTapeTiming.nomBlockTiming.trailerToneDuration;
    double first_block_gap = mTapeTiming.nomBlockTiming.firstBlockGap;
    double block_gap = mTapeTiming.nomBlockTiming.blockGap;
    double last_block_gap = mTapeTiming.nomBlockTiming.lastBlockGap;


    double high_tone_freq = mTapeTiming.baseFreq * 2;


    if (tapeFile.blocks.empty())
        return false;


    if (mVerbose)
        cout << "\nEncode program '" << tapeFile.blocks[0].atomHdr.name << "' as a CSW file...\n\n";


    FileBlockIter file_block_iter = tapeFile.blocks.begin();




    int block_no = 0;
    int n_blocks = (int)tapeFile.blocks.size();

    // Encode initial gap before first block
    if (!writeGap(first_block_gap)) {
        printf("Failed to encode a gap of %f s\n", block_gap);
    }

    if (mVerbose)
        cout << first_block_gap << " s GAP\n";


    while (file_block_iter < tapeFile.blocks.end()) {

        // Write a lead tone for the block
        if (mUseOriginalTiming) {
            prelude_tone_cycles = file_block_iter->preludeToneCycles;
            lead_tone_duration = (file_block_iter->leadToneCycles) / high_tone_freq;
            mTapeTiming.phaseShift = file_block_iter->phaseShift;
        }
        if (block_no == 0) {
            double prelude_lead_tone_duration = prelude_tone_cycles / high_tone_freq;
            if (!writeTone(prelude_lead_tone_duration)) {            
                printf("Failed to write prelude lead tone of duration %f s\n", prelude_lead_tone_duration);
            }

            if (!writeByte(0xaa, bbmDefaultDataEncoding)) {
                cout << "Failed to write dummy byte 0xaa\n";
            }
        }
        if (!writeTone(lead_tone_duration)) {
            printf("Failed to write (postlude) lead tone of duration %f s\n", lead_tone_duration);
        }

        if (mVerbose) {
            cout << dec;
            if (block_no > 0)
                cout << "BLOCK " << block_no << ": LEAD TONE " << lead_tone_duration << " s : ";
            else
                cout << "BLOCK 0: PRELUDE " << prelude_tone_cycles << " cycles : DUMMY BYTE : POSTLUDE " << lead_tone_duration << " s : ";
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
        if (!file_block_iter->encodeTapeHdr(header_data)) {
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
        BytesIter bi = file_block_iter->data.begin();
        Bytes block_data;
        while (bi < file_block_iter->data.end())
            block_data.push_back(*bi++);

        // Encode the block data bytes
        BytesIter data_iter = block_data.begin();
        while (data_iter < block_data.end())
            writeByte(*data_iter++, bbmDefaultDataEncoding);


        //
        // Write the data CRC
        //

        Word data_CRC = mCRC;
        if (!writeByte(data_CRC / 256, bbmDefaultDataEncoding) || !writeByte(data_CRC % 256, bbmDefaultDataEncoding)) {
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
                trailer_tone_duration = (file_block_iter->trailerToneCycles) / high_tone_freq;

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
                block_gap = file_block_iter->blockGap;

            if (!writeGap(block_gap)) {
                printf("Failed to encode a gap of %f s\n", block_gap);
                return false;
            }

            if (mVerbose)
                cout << block_gap << " s GAP";

        }

        if (mVerbose)
            cout << "\n";

        file_block_iter++;


        block_no++;

    }

    if (mVerbose)
        cout << mPulses.size() << " pulses created from Tape File!\n";

    if (mPulses.size() == 0) {
        cout << "No pulses could be created from Tape File!\n";
        return false;
    }



    // Write samples to CSW file
    if (!writeSamples(filePath)) {
        cout << "Failed to write CSW pulses to file '" << filePath << "'!\n";
        return false;
    }
 

    if (mVerbose)
        cout << "\nDone encoding program '" << tapeFile.blocks[0].atomHdr.name << "' as a CSW file...\n\n";

    return true;

}

bool CSWCodec::writeSamples(string filePath)
{
    // Write samples to CSW file
    CSW2Hdr hdr;
    ofstream fout(filePath, ios::out | ios::binary | ios::ate);

    // Write CSW header
    hdr.csw2.compType = 0x02; // (Z - RLE) - ZLIB compression of data stream
    hdr.csw2.flags = 0; // Initial polarity (specified by bit b0) is LOW
    hdr.csw2.hdrExtLen = 0;
    hdr.csw2.sampleRate[0] = mBitTiming.fS & 0xff;
    hdr.csw2.sampleRate[1] = (mBitTiming.fS >> 8) & 0xff;
    hdr.csw2.sampleRate[2] = (mBitTiming.fS >> 16) & 0xff;
    hdr.csw2.sampleRate[3] = (mBitTiming.fS >> 24) & 0xff;
    int n_pulses = (int)mPulses.size();
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
}

bool CSWCodec::encodeAtom(TapeFile& tapeFile, string &filePath)
{

    double lead_tone_duration = mTapeTiming.nomBlockTiming.firstBlockLeadToneDuration;
    double other_block_lead_tone_duration = mTapeTiming.nomBlockTiming.otherBlockLeadToneDuration;
    double data_block_micro_lead_tone_duration = mTapeTiming.nomBlockTiming.microLeadToneDuration;
    double first_block_gap = mTapeTiming.nomBlockTiming.firstBlockGap;
    double block_gap = mTapeTiming.nomBlockTiming.blockGap;
    double last_block_gap = mTapeTiming.nomBlockTiming.lastBlockGap;
    

    double high_tone_freq = mTapeTiming.baseFreq * 2;

 
    if (tapeFile.blocks.empty())
        return false;


    if (mVerbose)
        cout << "\nEncode program '" << tapeFile.blocks[0].atomHdr.name << "' as a CSW file...\n\n";


    FileBlockIter ATM_block_iter = tapeFile.blocks.begin();




    int block_no = 0;
    int n_blocks = (int)tapeFile.blocks.size();

    // Encode initial gap before first block
    if (!writeGap(first_block_gap)) {
        printf("Failed to encode a gap of %f s\n", block_gap);
    }

    if (mVerbose)
        cout << first_block_gap << " s GAP\n";


    while (ATM_block_iter < tapeFile.blocks.end()) {

        // Write a lead tone for the block
        if (mUseOriginalTiming) {
            lead_tone_duration = (ATM_block_iter->leadToneCycles) / high_tone_freq;
            mTapeTiming.phaseShift = ATM_block_iter->phaseShift;
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


        int data_len = ATM_block_iter->atomHdr.lenHigh * 256 + ATM_block_iter->atomHdr.lenLow;  // get data length

        Byte b7 = (block_no < n_blocks - 1 ? 0x80 : 0x00);          // calculate flags
        Byte b6 = (data_len > 0 ? 0x40 : 0x00);
        Byte b5 = (block_no != 0 ? 0x20 : 0x00);

        Bytes header_data;

        // store preamble
        for (int i = 0; i < 4; i++)
            header_data.push_back(0x2a);

        // store block name
        int name_len = 0;
        for (; name_len < sizeof(ATM_block_iter->atomHdr.name) && ATM_block_iter->atomHdr.name[name_len] != 0; name_len++);
        for (int i = 0; i < name_len; i++)
            header_data.push_back(ATM_block_iter->atomHdr.name[i]);

        header_data.push_back(0xd);

        header_data.push_back(b7 | b6 | b5);                        // store flags

        header_data.push_back((block_no >> 8) & 0xff);              // store block no
        header_data.push_back(block_no & 0xff);

        header_data.push_back((data_len > 0 ? data_len - 1 : 0));   // store length - 1

        header_data.push_back(ATM_block_iter->atomHdr.execAdrHigh);     // store execution address
        header_data.push_back(ATM_block_iter->atomHdr.execAdrLow);

        header_data.push_back(ATM_block_iter->atomHdr.loadAdrHigh);     // store load address
        header_data.push_back(ATM_block_iter->atomHdr.loadAdrLow);


        // Encode the header bytes
        BytesIter hdr_iter = header_data.begin();
        while (hdr_iter < header_data.end())
            writeByte(*hdr_iter++, atomDefaultDataEncoding);

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
            writeByte(*data_iter++, atomDefaultDataEncoding);

        //
        // Write CRC
        //

        // Encode CRC byte

        if (!writeByte(mCRC & 0xff, atomDefaultDataEncoding)) {
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
    hdr.csw2.sampleRate[0] = mBitTiming.fS & 0xff;
    hdr.csw2.sampleRate[1] = (mBitTiming.fS >> 8) & 0xff;
    hdr.csw2.sampleRate[2] = (mBitTiming.fS >> 16) & 0xff;
    hdr.csw2.sampleRate[3] = (mBitTiming.fS >> 24) & 0xff;
    int n_pulses = (int) mPulses.size();
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
        cout << "\nDone encoding program '" << tapeFile.blocks[0].atomHdr.name << "' as a CSW file...\n\n";

    return true;

}

bool CSWCodec::decode(string &CSWFileName, Bytes& pulses, HalfCycle& firstHalfCycle)
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
        mBitTiming.fS = csw2_hdr.sampleRate[0] + (csw2_hdr.sampleRate[1] << 8) + (csw2_hdr.sampleRate[2] << 16) + (csw2_hdr.sampleRate[3] << 24);
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
        mBitTiming.fS = csw1_hdr.sampleRate[0] + (csw1_hdr.sampleRate[1] << 8);
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
        cout << "sample frequency: " << (int)mBitTiming.fS << "\n";
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

bool CSWCodec::writeByte(Byte byte, DataEncoding encoding)
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

    FileBlock::updateCRC(mTargetMachine, mCRC, byte);

    return true;

}

bool CSWCodec::writeDataBit(int bit)
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

bool CSWCodec::writeStartBit()
{
    int n_cycles = mBitTiming.startBitCycles;

    if (!writeCycle(false, n_cycles)) {
        printf("Failed to encode start bit%s\n", "");
        return false;
    }

    return true;
}

bool CSWCodec::writeStopBit(DataEncoding encoding)
{
    int n_cycles = mBitTiming.stopBitCycles * encoding.nStopBits + (encoding.extraShortWave ? 1 : 0);


    if (!writeCycle(true, n_cycles)) {
        printf("Failed to encode stop bit%s\n", "");
        return false;
    }


    return true;
}

bool CSWCodec::writeTone(double duration)
{
    
    int n_cycles = (int)round((double)duration * mTapeTiming.baseFreq * 2);

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

    unsigned n_samples = (unsigned) round(duration * mBitTiming.fS);

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
        n_samples_per_half_cycle = mBitTiming.F2Samples / 2;
    }
    else {
        n_samples_per_half_cycle = mBitTiming.F1Samples / 2;
    }

    double sample_no = 0;
    double prev_sample_no = - n_samples_per_half_cycle;
    for (unsigned c = 0; c < n; c++) {

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

bool CSWCodec::setBaseFreq(double baseFreq)
{
    mTapeTiming.baseFreq = baseFreq;

    // Update bit timing as impacted by the change in Base frequency
    BitTiming new_bit_timing(mBitTiming.fS, mTapeTiming.baseFreq, mTapeTiming.baudRate, mTargetMachine);
    mBitTiming = new_bit_timing;

    return true;
}

bool CSWCodec::setBaudRate(int baudrate)
{
    mTapeTiming.baudRate = baudrate;

    // Update bit timing as impacted by the change in Baudrate
    BitTiming new_bit_timing(mBitTiming.fS, mTapeTiming.baseFreq, mTapeTiming.baudRate, mTargetMachine);
    mBitTiming = new_bit_timing;

    return true;
}

bool CSWCodec::setPhase(int phase)
{
    //mBitTiming.
    mTapeTiming.phaseShift = phase;
    return true;
}