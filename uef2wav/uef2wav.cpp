
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "../shared/CommonTypes.h"
#include "ArgParser.h"
#include "../shared/WavEncoder.h"
#include "../shared/UEFCodec.h"
#include "../shared/Debug.h"
#include "../shared/Utility.h"

using namespace std;
using namespace std::filesystem;



/*
 *
 * Create WAV file from UEF file
 * *
 */
int main(int argc, const char* argv[])
{


    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    UEFCodec UEF_codec = UEFCodec(arg_parser.verbose, arg_parser.targetMachine);

    WavEncoder WAV_encoder = WavEncoder(arg_parser.mSampleFreq, arg_parser.tapeTiming, arg_parser.verbose, arg_parser.targetMachine);
        

    // Read UEF file chunks
    if (!UEF_codec.readUefFile(arg_parser.srcFileName)) {
        cout << "Failed to read UEF File\n";
        return -1;
    }

    // Iterate over read UEF file chunks and write them to CSW file
    ChunkInfo chunk_info;
    while (UEF_codec.processChunk(chunk_info)) {
        switch (chunk_info.chunkInfoType) {
        case ChunkInfoType::CARRIER_DUMMY:
            if (!WAV_encoder.writeTone(chunk_info.data1_fp))
                return false;
            if (!WAV_encoder.writeByte(0xaa, bbmDefaultDataEncoding)) // Only BBC Machines uses dummy bytes
                return false;
            if (!WAV_encoder.writeTone((double)chunk_info.data2_i / (2 * UEF_codec.getBaseFreq())))
                return false;
            if (arg_parser.verbose)
                cout << "wrote a carrier with dummy byte 0xaa of duration " <<
                chunk_info.data1_fp << "s + " << (double)chunk_info.data2_i / (2 * UEF_codec.getBaseFreq()) << "s\n";
            break;
        case ChunkInfoType::CARRIER:
            if (!WAV_encoder.writeTone(chunk_info.data1_fp))
                return false;
            if (arg_parser.verbose)
                cout << "Wrote a carrier of duration " << chunk_info.data1_fp << "s\n";
            break;
        case ChunkInfoType::DATA:
            for (int i = 0; i < chunk_info.data.size(); i++) {
                if (!WAV_encoder.writeByte(chunk_info.data[i], chunk_info.dataEncoding))
                    return false;
            }
            if (arg_parser.verbose)
                cout << "Wrote " << dec << chunk_info.data.size() << " bytes\n";
            break;
        case ChunkInfoType::GAP:
            if (!WAV_encoder.writeGap(chunk_info.data1_fp))
                return false;
            if (arg_parser.verbose)
                cout << "Wrote a gap of duration " << chunk_info.data1_fp << "s\n";
            break;
        case ChunkInfoType::BASE_FREQ:
            if (!WAV_encoder.setBaseFreq(chunk_info.data1_fp))
                return false;
            if (arg_parser.verbose)
                cout << "Wrote a base frequency of " << chunk_info.data1_fp << " Hz\n";
            break;
        case ChunkInfoType::BAUDRATE:
            if (!WAV_encoder.setBaudRate(chunk_info.data2_i))
                return false;
            if (arg_parser.verbose)
                cout << "Wrote a Baudrate of " << chunk_info.data2_i << "\n";
            break;
        case ChunkInfoType::PHASE:
            if (!WAV_encoder.setPhase(chunk_info.data2_i))
                return false;
            if (arg_parser.verbose)
                cout << "Wrote a phase of " << chunk_info.data2_i << "\n";
            break;
        default:
            break;
        }
    }

    if (!WAV_encoder.writeSamples(arg_parser.dstFileName)) {
        cout << "Failed to write read UEF data to WAV file\n";
        return -1;
    }

    return 0;
}



