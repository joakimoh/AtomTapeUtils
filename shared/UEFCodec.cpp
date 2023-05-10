#include <gzstream.h>
#include <stdlib.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string.h>
#include "UEFCodec.h"
#include "CommonTypes.h"
#include "BlockTypes.h"
#include "Debug.h"
#include "Utility.h"
#include "BlockTypes.h"



using namespace std;


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

UEFCodec::UEFCodec(bool verbose) : mVerbose(verbose)
{

}

UEFCodec::UEFCodec(TAPFile& tapFile, bool useOriginalTiming, bool verbose): mTapFile(tapFile), mVerbose(verbose)
{
    mUseOriginalTiming = useOriginalTiming;
}


bool UEFCodec::setTapeTiming(TapeProperties tapeTiming)
{
    mTapeTiming = tapeTiming;
    return true;
}

typedef struct BLOCK_INFO_struct {
    int lead_tone_cycles;
    int micro_tone_cycles;
    double block_gap;
    int phase = 180;
    double start;
    double end;
} BLOCK_INFO;

typedef enum { WAIT_LEAD_TONE, WAIT_HDR, READING_HDR, WAIT_MICRO_TONE, WAIT_DATA, READING_DATA, WAIT_GAP, UNDEFINED_BLOCK_STATE
} BLOCK_STATE;

#define _BLOCK_STATE(x) \
    (x==WAIT_LEAD_TONE?"WAIT_LEAD_TONE":\
    (x==WAIT_HDR?"WAIT_HDR":(x==WAIT_MICRO_TONE?"WAIT_MICRO_TONE":\
    (x==WAIT_DATA?"WAIT_DATA":(x==WAIT_GAP?"WAIT_GAP":(x==READING_HDR?"READING_HDR":\
    (x==READING_DATA?"READING_DATA":"???")))))))
bool UEFCodec::encode(string &filePath)
{
    if (mTapFile.blocks.empty()) 
        return false;

    ogzstream  fout(filePath.c_str());
    if (!fout.good()) {
        printf("Can't write to UEF file '%s'...\n", filePath.c_str());
        return false;
    }

   ATMBlockIter ATM_block_iter = mTapFile.blocks.begin();
 

    int block_no = 0;
    int n_blocks = (int)mTapFile.blocks.size();

    

    /*
     * Timing parameters
     *
     * Mostly based on observed timing in Acorn Atom tape recordings but
     * in some cases changed as Atomulator emulator won't wok otherwise.
     *
     */
    int baudrate = mTapeTiming.baudRate;
    if (mUseOriginalTiming && mTapFile.validTiming)
        baudrate = mTapFile.baudRate;

    float base_freq = mTapeTiming.baseFreq;
    float high_tone_freq = 2 * base_freq;


    // Default values for block timing (if mUseOriginalTiming is true the recored block timing will instead by used later on)
    float first_block_lead_tone_duration = mTapeTiming.nomBlockTiming.firstBlockLeadToneDuration;// lead tone duration of first block 
    float other_block_lead_tone_duration = mTapeTiming.nomBlockTiming.otherBlockLeadToneDuration;// lead tone duration of all other blocks (2 s expected here but Atomulator needs 4 s)
    float data_block_micro_lead_tone_duration = mTapeTiming.nomBlockTiming.microLeadToneDuration; //  micro lead tone (separatiing block header and block data) duration
    float first_block_gap = mTapeTiming.nomBlockTiming.firstBlockGap;
    float block_gap = mTapeTiming.nomBlockTiming.blockGap;
    float last_block_gap = mTapeTiming.nomBlockTiming.lastBlockGap;  
    float phase = mTapeTiming.phase;
    float lead_tone_duration = first_block_lead_tone_duration; // let first block have a longer lead tone


    //
    // Write data-independent part of UEF file
    //


    // Header: UEF File!
    UefHdr hdr;
    fout.write((char*)&hdr, sizeof(hdr));


    // Add origin
    string origin = "Joakim Ohlsson";
    if (!writeOriginChunk(fout, origin)) {
        printf("Failed to write origin chunk with string %s\n", origin.c_str());
    }

 
    // Baudrate
    if (!writeBaudrateChunk(fout, baudrate)) {
        printf("Failed to write buadrate chunk with baudrate %d.\n", baudrate);
    }


    // Phase 0
    if (!writePhaseChunk(fout, phase)) {
        printf("Failed to write phase chunk with phase %d degrees\n", phase);
    }




    // Write an intial gap before the file starts
    if (!writeFloatPrecGapChunk(fout, first_block_gap)) {
        printf("Failed to write %f s gap\n", first_block_gap);
    }



    //
    // Write data-dependent part of UEF file
    //


    

    while (ATM_block_iter < mTapFile.blocks.end()) {



        // Write base frequency
        if (!writeBaseFrequencyChunk(fout, base_freq)) {
            printf("Failed to encode base frequency %f as float\n", base_freq);
        }

        // Write a lead tone for the block
        if (mUseOriginalTiming && mTapFile.validTiming)
            lead_tone_duration = (ATM_block_iter->leadToneCycles) / high_tone_freq;
        if (!writeHighToneChunk(fout, lead_tone_duration, baudrate)) {
            printf("Failed to write %f Hz lead tone of duration %f s\n", base_freq, lead_tone_duration);
        }
;

        // Change lead tone duration for remaining blocks
        lead_tone_duration = other_block_lead_tone_duration;


        // Write 1 security cycle
        /*
        if (!writeSecurityCyclesChunk(fout, 1, 'P', 'W', { 0 })) {
            printf("Failed to write security bytes\n");
        }
        */
 
        // Write (again the) base frequency 
        if (!writeBaseFrequencyChunk(fout, base_freq)) {
            printf("Failed to encode base frequency %f as float\n", base_freq);
        }

        // Initialise CRC

        Byte CRC = 0;

        // --------------------- start of block header chunk ------------------------
        //
        // Write block header including preamble as one chunk
        //
        // block header: <flags:1> <block_no:2> <length-1:1> <exec addr:2> <load addr:2>
        // 
        // --------------------------------------------------------------------------


        int data_len = ATM_block_iter->hdr.lenHigh * 256 + ATM_block_iter->hdr.lenLow;  // get data length

        Byte b7 = (block_no < n_blocks - 1 ? 0x80 : 0x00);          // calculate flags
        Byte b6 = (data_len > 0 ? 0x40 : 0x00);
        Byte b5 = (block_no != 0 ? 0x20 : 0x00);

        Bytes header_data;

        Byte preamble[4] = { 0x2a, 0x2a, 0x2a, 0x2a };              // store preamble
        addBytes2Vector(header_data, preamble, 4);

        int name_len = 0;
        for (; name_len < sizeof(ATM_block_iter->hdr.name) && ATM_block_iter->hdr.name[name_len] != 0; name_len++);
        addBytes2Vector(header_data, (Byte*)ATM_block_iter->hdr.name, name_len); // store block name

        header_data.push_back(0xd);

        header_data.push_back(b7 | b6 | b5);                        // store flags

        header_data.push_back((block_no >> 8) & 0xff);              // store block no
        header_data.push_back(block_no & 0xff);

        header_data.push_back((data_len > 0 ? data_len - 1 : 0));   // store length - 1

        header_data.push_back(ATM_block_iter->hdr.execAdrHigh);     // store execution address
        header_data.push_back(ATM_block_iter->hdr.execAdrLow);

        header_data.push_back(ATM_block_iter->hdr.loadAdrHigh);     // store load address
        header_data.push_back(ATM_block_iter->hdr.loadAdrLow);


        if (!writeData104Chunk(fout, 8, 'N', -1, header_data, CRC)) {
            printf("Failed to write block header data chunk%s\n", "");
        }




        

        // --------------------------------------------------------------------------
        //
        // ---------------- End of block header chunk -------------------------------


        // Add micro tone between header and data
        if (mUseOriginalTiming && mTapFile.validTiming)
            data_block_micro_lead_tone_duration = ATM_block_iter-> microToneCycles / high_tone_freq;
        if (!writeHighToneChunk(fout, data_block_micro_lead_tone_duration, baudrate)) {
            printf("Failed to write %f Hz micro lead tone of duration %f s\n", base_freq, data_block_micro_lead_tone_duration);
        }


        // Write (again the) base frequency 
        if (!writeBaseFrequencyChunk(fout, base_freq)) {
            printf("Failed to encode base frequency %f as float\n", base_freq);
        }


        // --------------------- start of block data + CRC chunk ------------------------
         //
         // Write block data as one chunk
         //
         // --------------------------------------------------------------------------

        // Write block data as one chunk
        BytesIter bi = ATM_block_iter->data.begin();
        Bytes block_data;
        while (bi < ATM_block_iter->data.end())
            block_data.push_back(*bi++);
        if (!writeData104Chunk(fout, 8, 'N', -1, block_data, CRC)) {
            printf("Failed to write block header data chunk%s\n", "");
        }


        // Write (again the) base frequency 
        if (!writeBaseFrequencyChunk(fout, base_freq)) {
            printf("Failed to encode base frequency %f as float\n", base_freq);
        }


        //
        // Write CRC
        //

        // Write CRC chunk header
        Bytes CRC_data;
        Byte dummy = 0;
        CRC_data.push_back(CRC);
        if (!writeData100Chunk(fout, CRC_data, dummy)) {
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
        if (mUseOriginalTiming && mTapFile.validTiming)
            block_gap = ATM_block_iter->blockGap;
        if (!writeFloatPrecGapChunk(fout, block_gap)) {
            printf("Failed to write %f s and of block gap\n", block_gap);
        }

        ATM_block_iter++;


        block_no++;

    }


    fout.close();

    if (!fout.good()) {
        printf("Failed writing to file %s for compression output\n", filePath.c_str());
        return false;
    }


    return true;

}

bool UEFCodec::decode(string &uefFileName)
{
    igzstream fin(uefFileName.c_str());

    if (!fin.good()) {
        printf("Failed to open file '%s'\n", uefFileName.c_str());
        return false;
    }

    if (mVerbose)
        cout << "\nDecode UEF file\n\n";

    filesystem::path fin_p = uefFileName;
    string file_name = fin_p.stem().string();

    mTapFile.blocks.clear();
    mTapFile.complete = true;
    mTapFile.validFileName = blockNameFromFilename(file_name);
    mTapFile.isAbcProgram = true;



    BLOCK_STATE block_state = WAIT_GAP;
    BLOCK_STATE prev_block_state = UNDEFINED_BLOCK_STATE;
    vector<BLOCK_INFO> block_info;
    BLOCK_INFO current_block_info;
    bool start = true;

    if (mVerbose)
        cout << "Initial block reading state is " << _BLOCK_STATE(block_state) << "...\n";
 
    Bytes data;

    UefHdr hdr;

    fin.read((char*)&hdr, sizeof(hdr));

    if (strcmp(hdr.uefTag, "UEF File!") != 0) {
        printf("File '%s' has no valid UEF header\n", uefFileName.c_str());
        fin.close();
        return false;
    }
    if (mVerbose)
        cout << "UEF Header with major version " << (int) hdr.uefVer[1] << " and minor version " << (int)  hdr.uefVer[0] << " read.\n";

    ChunkHdr chunk_hdr;
    int baud_rate = 300;
    int base_freq = 1200;
    double current_time = 0;

    while (fin.read((char *) &chunk_hdr, sizeof(chunk_hdr))) {

        int chunk_id = chunk_hdr.chunkId[0] + chunk_hdr.chunkId[1] * 256;
        long chunk_sz = chunk_hdr.chunkSz[0] + (chunk_hdr.chunkSz[1] << 8) + (chunk_hdr.chunkSz[2] << 16) + (chunk_hdr.chunkSz[3] << 24);

        switch (chunk_id) {

            case 0x0116: // Floating-point gap
            {
                if (chunk_sz != 4) {
                    printf("Size of Format change chunk 0116 has an incorrect chunk size %ld (should have been %d)\n", chunk_sz, 4);
                    return false;
                }
                FPGap0116 gap_chunk;
                fin.read((char*)gap_chunk.gap, sizeof(gap_chunk.gap));
                float gap;

                if (!decodeFloat(gap_chunk.gap, gap)) {
                    printf("Failed to decode IEEE 754 gap%s\n", "");
                }
                if (mVerbose)
                    cout << "Format change chunk 0116 of size " << chunk_sz << " and specifying a gap of " << gap << " s.\n";
                if (block_state == BLOCK_STATE::WAIT_GAP || block_state == BLOCK_STATE::READING_DATA) {
                    if (!start) {
                        current_block_info.block_gap = gap;
                        block_info.push_back(current_block_info);
                    }
                    else
                        start = false;
                    block_state = BLOCK_STATE::WAIT_LEAD_TONE;
                    if (mVerbose && block_state != prev_block_state)
                        cout << "Block reading state changed to " << _BLOCK_STATE(block_state) << "...\n";
                }
                else {
                    printf("Block gap when block state was %s!\n", _BLOCK_STATE(block_state));
                    return false;
                }
                current_block_info.end = current_time;
                current_time += gap;
                break;
            }

            case 0x0113: // Base frequency
            {              
                if (chunk_sz != 4) {
                    printf("Size of Format change chunk 0113 has an incorrect chunk size %ld (should have been %d)\n", chunk_sz, 4);
                    return false;
                }
                BaseFreqChangeChunk0113 chunk;
                fin.read((char*)chunk.frequency, sizeof(chunk.frequency));
                float f;
                if (!decodeFloat(chunk.frequency, f)) {
                    printf("Failed to decode IEEE 754 gap%s\n", "");
                }
                if (mVerbose)
                    cout << "Format change chunk 0113 of size " << chunk_sz << " and specifying a frequency of " << f << ".Hz\n";
                base_freq = f;
                break;
            }


            case 0x117: // Format Change Chunk 0117: baudrate = value
            {          
                if (chunk_sz != 2) {
                    printf("Size of Format change chunk 0117 has an incorrect chunk size %ld (should have been %d)\n", chunk_sz, 2);
                    return false;
                }
                DataFormatChange0117 baudrate_chunk;
                fin.read((char*)baudrate_chunk.baudRate, sizeof(baudrate_chunk.baudRate));
                int baudrate = (baudrate_chunk.baudRate[0] + (baudrate_chunk.baudRate[1] << 8));
                if (mVerbose)
                    cout << "Format change chunk 0117 of size " << chunk_sz << " and baudrate of " << baudrate << ".\n";

                break;
            }
            case 0x0115: // Phase Change Chunk 0115: phase = value
            {
                if (chunk_sz != 2) {
                    printf("Size of Phase change chunk 0115 has an incorrect chunk size %ld (should have been %d)\n", chunk_sz, 2);
                    return false;
                }
                PhaseChangeChunk0115 phase_chunk;
                phase_chunk.chunkHdr = chunk_hdr;
                fin.read((char*)phase_chunk.phase, sizeof(phase_chunk.phase));
                int phase = (phase_chunk.phase[0] + (phase_chunk.phase[1] << 8));
                if (mVerbose)
                    cout << "Phase change chunk 0115 of size " << chunk_sz << " and specifiying a phase of " << phase << " degrees.\n";

                current_block_info.phase = phase;
                break;
            }

            case 0x0112: // Baudwise Gap Chunk 0112: gap = value / (baud_rate  * 2)
            {
                if (chunk_sz != 2) {
                    printf("Size of Baudwise gap chunk 0112 has an incorrect chunk size %ld (should have been %d)\n", chunk_sz, 2);
                    return false;
                }
                BaudwiseGapChunk0112 gap_chunk;
                gap_chunk.chunkHdr = chunk_hdr;
                fin.read((char*)gap_chunk.gap, sizeof(gap_chunk.gap));
                double gap_duration = double (gap_chunk.gap[0] + (gap_chunk.gap[1] << 8)) / (baud_rate * 2);
                if (mVerbose)
                    cout << "Baudwise gap chunk 0112 of size " << chunk_sz << " and specifiying a duration of " << gap_duration << " s.\n";
                if (block_state == BLOCK_STATE::WAIT_GAP|| block_state == BLOCK_STATE::READING_DATA) {
                    if (!start) {
                        current_block_info.block_gap = gap_duration;
                        block_info.push_back(current_block_info);
                    }
                    else
                        start = false;
                    block_state = BLOCK_STATE::WAIT_LEAD_TONE;
                    if (mVerbose && block_state != prev_block_state)
                        cout << "Block reading state changed to " << _BLOCK_STATE(block_state) << "...\n";
                }
                else {
                    printf("Block gap when block state was %s!\n", _BLOCK_STATE(block_state));
                    return false;
                }
                current_block_info.end = current_time;
                current_time += gap_duration;
                break;
            }

            case 0x0110: // High Tone Chunk 0110: duration = value / (baud_rate * 2)
            {
                if (chunk_sz != 2) {
                    printf("Size of High tone chunk 0110 has an incorrect chunk size %ld (should have been %d)\n", chunk_sz, 2);
                    return false;
                }
                HighToneChunk0110 tone_chunk;
                tone_chunk.chunkHdr = chunk_hdr;
                fin.read((char*)tone_chunk.duration, sizeof(tone_chunk.duration));
                double tone_duration = double (tone_chunk.duration[0] + (tone_chunk.duration[1] << 8)) / (baud_rate * 2);
                if (mVerbose)
                    cout << "High tone chunk 0110 of size " << chunk_sz << " and specifiying a duration of " << tone_duration << " s.\n";
                if (block_state == BLOCK_STATE::WAIT_LEAD_TONE) {
                    current_block_info.lead_tone_cycles = tone_duration * base_freq *2;
                    block_state = BLOCK_STATE::WAIT_HDR;
                    current_block_info.start = current_time;
                }
                else if (block_state == BLOCK_STATE::WAIT_MICRO_TONE|| block_state == BLOCK_STATE::READING_HDR) {
                    current_block_info.micro_tone_cycles = tone_duration * base_freq * 2;
                    block_state = BLOCK_STATE::WAIT_DATA;
                }
                else {
                    printf("High tone when block state was %s!\n", _BLOCK_STATE(block_state));
                    return false;
                }
                if (mVerbose && block_state != prev_block_state)
                    cout << "Block reading state changed to " << _BLOCK_STATE(block_state) << "...\n";
                current_time += tone_duration;
                break;
            }


            case 0x0114: // Security cycles
            {
                SecurityCycles0114Hdr hdr;
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
                    cout << "Security cycles chunk 0114 of size " << chunk_sz << " specifiying " << n_cycles;
                    cout << " with format " << hdr.firstPulse << hdr.lastPulse << " and cycle bytes " << cycle_bytes << ".\n";
                }

                break;
            }

            case 0x0000: // Origin Chunk 0000
            {
                string origin = "";
                for (int pos = 0; pos < chunk_sz; pos++) {
                    char c;
                    fin.get(c);
                    origin += c;
                }
                if (mVerbose)
                    cout << "Origin chunk 000 of size " << chunk_sz << "  and text '" << origin << "'.\n";
                break;
            }

            case 0x0100: // Data Block Chunk 0100
            {
                if (mVerbose)
                    cout << "Data block chunk 0100 of size " << chunk_sz << ".\n";
                for (int i = 0; i < chunk_sz; i++) {
                    Byte byte;
                    fin.read((char*)&byte, 1);
                    data.push_back(byte);
                }
                if (block_state == BLOCK_STATE::WAIT_HDR || block_state == BLOCK_STATE::READING_HDR) {
                    block_state = BLOCK_STATE::READING_HDR;
                }
                else if (block_state == BLOCK_STATE::WAIT_DATA || block_state == BLOCK_STATE::READING_DATA) {
                    block_state = BLOCK_STATE::READING_DATA;
                }
                else {
                    printf("Bytes when block state was %s!\n", _BLOCK_STATE(block_state));
                    return false;
                }
                if (mVerbose && block_state != prev_block_state)
                    cout << "Block reading state changed to " << _BLOCK_STATE(block_state) << "...\n";
                current_time += chunk_sz * 10 * 4 / base_freq;
                break;
            }
             
             case 0x0104: // Data Block Chunk 0104
            {
                DataBlockChunk0104Hdr hdr;
                hdr.chunkHdr = chunk_hdr;
                fin.read((char*) &hdr.bitsPerPacket, sizeof(hdr.bitsPerPacket));
                fin.read((char*) &hdr.parity, sizeof(hdr.parity));
                fin.read((char*) &hdr.stopBitInfo, sizeof(hdr.stopBitInfo));
                int nStopBits = (hdr.stopBitInfo >= 0x80 ? hdr.stopBitInfo - 256 : hdr.stopBitInfo);

                if (mVerbose) {
                    cout << "Data block chunk 0104 of size " << chunk_sz << " and packet format ";
                    cout << (int) hdr.bitsPerPacket << (char) hdr.parity << (int) nStopBits << "\n";
                }
                for (int i = 0; i < chunk_sz - 3; i++) {
                    Byte byte;
                    fin.read((char*)&byte, 1);
                    data.push_back(byte);
                }
                if (block_state == BLOCK_STATE::WAIT_HDR || block_state == BLOCK_STATE::READING_HDR) {
                    block_state = BLOCK_STATE::READING_HDR;
                }
                else if (block_state == BLOCK_STATE::WAIT_DATA || block_state == BLOCK_STATE::READING_DATA) {
                    block_state = BLOCK_STATE::READING_DATA;
                }
                else {
                    printf("Bytes when block state was %s!\n", _BLOCK_STATE(block_state));
                    return false;
                }
                if (mVerbose && block_state != prev_block_state)
                    cout << "Block reading state changed to " << _BLOCK_STATE(block_state) << "...\n";
                current_time += chunk_sz * 10 * 4 / base_freq;
                break;
            }

 
            case 0x0002:
            
            default: // Unknown chunk
            {
                printf("Unknown chunk %.4x of size %ld.\n", chunk_id, chunk_sz);
                fin.ignore(chunk_sz);
                break;
            }

        }

        prev_block_state = block_state;
    }

    fin.close();

    if (!fin.eof()) {
        printf("Failed while reading compressed input file %s\n", uefFileName.c_str());
        return false;
    }

    if (mVerbose)
        cout << "\n\nUEF File bytes read - now to decode them as an Atom Tape File\n\n";



    //
    // Iterate over the collected data and create an Atom File with ATM blocks from it
    //
    
    int block_no = 0;
    BytesIter data_iter = data.begin();
    BlockType block_type;
    string tape_file_name;

    while (data_iter < data.end()) {

        //
        // Read one block
        // 

        ATMBlock block;
        Byte CRC = 0;
        BytesIter hdr_start = data_iter;

        block.blockGap = block_info[block_no].block_gap;
        block.leadToneCycles = block_info[block_no].lead_tone_cycles;
        block.microToneCycles = block_info[block_no].micro_tone_cycles;
        block.phaseShift = block_info[block_no].phase;
        block.tapeStartTime = block_info[block_no].start;
        block.tapeEndTime = block_info[block_no].end;
     
        // Read preamble (4 x 0x2a)
        if (!check_bytes(data_iter, data, 4, CRC, 0x2a)) {
            printf("Failed to read preamble bytes for block #%d.\n", block_no);
            return false;
        }


        // Read block name (up to 13 chars terminated by 0xd)
        if (!read_block_name(data_iter, data, CRC, block.hdr.name)) {
            printf("Failed to read name bytes for block #%d.\n", block_no);
            return false;
        }


        // Read remaining of block header
        AtomTapeBlockHdr hdr;
        if (!read_bytes(data_iter, data, sizeof(hdr), CRC, (Byte *)  & hdr)) {
            printf("Failed to read last part of header  for block #%d '%s'.\n", block_no, block.hdr.name);
            return false;
        }


       

        int data_len;
        block_type = parseBlockFlag(hdr, data_len);

        block.hdr.execAdrHigh = hdr.execAdrHigh;
        block.hdr.execAdrLow = hdr.execAdrLow;
        block.hdr.lenHigh = data_len / 256;
        block.hdr.lenLow = data_len % 256;
        block.hdr.loadAdrHigh = hdr.loadAdrHigh;
        block.hdr.loadAdrLow = hdr.loadAdrLow;

        if (mVerbose)
            printf("%s %.4x %.4x %.4x %.2x %.2x %.7s\n", block.hdr.name, hdr.loadAdrHigh * 256 + hdr.loadAdrLow,
            hdr.execAdrHigh * 256 + hdr.execAdrLow, hdr.blockNoHigh * 256 + hdr.blockNoLow, hdr.dataLenM1,
            hdr.flags, _BLOCK_ORDER(block_type)
        );

        logData(0x0000, hdr_start, data_iter - hdr_start + 1);

        // Read block data
        if (!read_bytes(data_iter, data, data_len, CRC, block.data)) {
            printf("Failed to read data part of block #%d '%s'.\n", block_no, block.hdr.name);
            return false;
        }
        BytesIter bi = block.data.begin();
        int load_address = hdr.loadAdrHigh * 256 + hdr.loadAdrLow;
        logData(load_address, bi, block.data.size());
        

        // Read CRC
        if (!(data_iter < data.end())) {
            printf("Failed to read CRC for block #%d '%s'.\n", block_no, block.hdr.name);
            return false;
        }
        Byte read_CRC = *data_iter++;

        // Check CRC
        if (read_CRC != CRC) {
            printf("Incorrect CRC 0x%.2x (expected 0x%.2x) for block #%d '%s'.\n", read_CRC, CRC, block_no, block.hdr.name);
            return false;
        }
 
        mTapFile.blocks.push_back(block);

        if (block_type & BlockType::First)
            tape_file_name = block.hdr.name;
        else if (tape_file_name != block.hdr.name) {
            printf("Name of block #%d '%s' not consistent with tape file name '%s'.\n", block_no, block.hdr.name, tape_file_name.c_str());
            return false;
        }

        block_no++;

    }

    if (!(block_type & BlockType::Last)) {
        printf("Missing last block for Atom File '%s'.\n", tape_file_name.c_str());
        return false;
    }

    fin.close();

    return true;
}

bool UEFCodec::read_bytes(BytesIter& data_iter, Bytes& data, int n, Byte & CRC, Bytes &block_data)
{
    int i = 0;

    for (i = 0; i < n && data_iter < data.end(); data_iter++) {
        block_data.push_back(*data_iter);
        CRC += *data_iter;
        i++;
    }

    if (i < n) {
        printf("Only %d of expected %d UEF bytes were read.\n", i, n);
        return false;
    }

    return true;
}

bool UEFCodec::read_bytes(BytesIter& data_iter, Bytes& data, int n, Byte & CRC, Byte bytes[])
{
    int i = 0;

    for (i = 0; i < n && data_iter < data.end(); data_iter++) {
        bytes[i] = *data_iter;
        CRC += *data_iter;
        i++;
    }

    if (i < n) {
        printf("Only %d of expected %d UEF bytes were read.\n", i, n);
        return false;
    }

    return true;
}

bool UEFCodec::check_bytes(BytesIter& data_iter, Bytes& data, int n, Byte & CRC, Byte refVal)
{
    int i = 0;

    for (i = 0; i < n && data_iter < data.end(); data_iter++) {
        if (*data_iter != refVal) {
            printf("UEF Byte %d was %x when expecting %d.\n", i, *data_iter, refVal);
            return false;
        }
        CRC += *data_iter;
        i++;
    }

    if (i < n) {
        printf("Only %d of expected %d UEF bytes were read.\n", i, n);
        return false;
    }

    return true;
}

bool UEFCodec::read_block_name(BytesIter &data_iter, Bytes &data, Byte& CRC, char name[ATM_MMC_HDR_NAM_SZ])
{
    int i = 0;

    for (i = 0; i < ATM_MMC_HDR_NAM_SZ && *data_iter != 0xd && data_iter < data.end(); data_iter++) {
        name[i] = *data_iter;
        CRC += *data_iter;
        i++;
    }
    for (int j = i; j < ATM_MMC_HDR_NAM_SZ; name[j++] = '\0');

    if (*data_iter != 0xd)
        return false;

    CRC += *data_iter++; // should be 0xd;

    return true;
}



bool UEFCodec::writeUEFHeader(ogzstream&fout, Byte majorVersion, Byte minorVersion)
{
    // Header: UEF File!
    UefHdr hdr;
    fout.write((char*)&hdr, sizeof(hdr));

    return true;
}

 bool UEFCodec::writeBaseFrequencyChunk(ogzstream&fout, float baseFrequency)
{
     BaseFreqChangeChunk0113 chunk;
     if (!encodeFloat(baseFrequency, chunk.frequency)) {
         printf("Failed to encode base frequency %f as float.\n", baseFrequency);
         return false;
     }
     fout.write((char*) &chunk, sizeof(chunk));

     if (mVerbose)
         printf("Base frequency chunk 0113 specifying %.0f Hz written.\n", baseFrequency);

     return true;
}

 // Write a float-precision gap 
bool UEFCodec::writeFloatPrecGapChunk(ogzstream&fout, float duration)
{
    
    FPGap0116 chunk;

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
bool UEFCodec::writeIntPrecGapChunk(ogzstream& fout, float duration, int baudRate)
{
    BaudwiseGapChunk0112 chunk;
    int gap = (int) round(duration * baudRate * 2);
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
    SecurityCycles0114Hdr hdr;


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
string UEFCodec::decode_security_cycles(SecurityCycles0114Hdr &hdr, Bytes cycles) {

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
bool UEFCodec::writeBaudrateChunk(ogzstream&fout, int baudrate)
{

    DataFormatChange0117 chunk;
    chunk.baudRate[0] = baudrate & 0xff;
    chunk.baudRate[1] = baudrate >> 8;
    fout.write((char*)&chunk, sizeof(chunk));

    if (mVerbose)
        printf("Baudrate chunk 0117 specifying a baud rate of %d written.\n", baudrate);

    return true;
}

// Write phase chunk
bool UEFCodec::writePhaseChunk(ogzstream &fout, int phase)
{

    PhaseChangeChunk0115 chunk;
    chunk.phase[0] = phase & 0xff;
    chunk.phase[1] = phase >> 8;
    fout.write((char*)&chunk, sizeof(chunk));

    if (mVerbose)
        printf("Phase chunk 0115 specifying a phase of %d degrees written.\n", phase);
    return true;
}

bool UEFCodec::writeHighToneChunk(ogzstream &fout, float duration, int baudRate)
{
    // Write a high tone
    HighToneChunk0110 chunk;
    int cycles = (int) round(duration * baudRate * 2);
    chunk.duration[0] = cycles % 256;
    chunk.duration[1] = cycles / 256;
    fout.write((char*)&chunk, sizeof(chunk));

    if (mVerbose)
        printf("High tone chunk 0110 specifying a %.2f s tone written.\n", duration);

    return true;
}

bool UEFCodec::writeData104Chunk(ogzstream &fout, Byte bitsPerPacket, Byte parity, Byte stopBitInfo, Bytes data, Byte & CRC)
{

    DataBlockChunk0104Hdr chunk;
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
        CRC += b;
        fout.write((char*)&b, sizeof(b));
    }

    if (mVerbose)
        printf("Data chunk 0104 of format %d%c%d and size %d written.\n", bitsPerPacket, parity, (stopBitInfo>=128?stopBitInfo-256: stopBitInfo), block_size);
    return true;
}

bool UEFCodec::writeData100Chunk(ogzstream &fout, Bytes data, Byte &CRC)
{
    DataBlockChunk0100Hdr chunk;
    int block_size = data.size();
    chunk.chunkHdr.chunkSz[0] = block_size & 0xff;
    chunk.chunkHdr.chunkSz[1] = (block_size >> 8) & 0xff;
    chunk.chunkHdr.chunkSz[2] = (block_size >> 16) & 0xff;
    chunk.chunkHdr.chunkSz[3] = (block_size >> 23) & 0xff;
    fout.write((char*)&chunk, sizeof(chunk));

    BytesIter data_iter = data.begin();
    for (int i = 0; i < block_size; i++) {
        Byte b = *data_iter++;
        CRC += b;
        fout.write((char*)&b, sizeof(b));
    }

    if (mVerbose)
        printf("Data chunk 0100 of size %d written.\n", block_size);

    return true;
}

// Write origin chunk
bool UEFCodec::writeOriginChunk(ogzstream &fout, string origin)
{
 
    OriginChunkHdr0000 chunk_hdr;
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

bool UEFCodec::getTAPFile(TAPFile& tapFile)
{
    tapFile = mTapFile;

    return true;
}

bool UEFCodec::setTAPFile(TAPFile& tapFile)
{
    mTapFile = tapFile;

    return true;
}
