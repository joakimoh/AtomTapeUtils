
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include <math.h>

#include "../shared/CommonTypes.h"
#include "ArgParser.h"
#include "../shared/UEFCodec.h"
#include "../shared/CSWCodec.h"


using namespace std;
using namespace std::filesystem;



/*
 * 
 * Create CSW file from UEF file
 * 
 * 
 */
int main(int argc, const char* argv[])
{
    

    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;

    UEFCodec UEF_codec = UEFCodec(arg_parser.verbose, arg_parser.targetMachine);

    CSWCodec CSW_codec = CSWCodec(arg_parser.mPreserveOriginalTiming, arg_parser.mSampleFreq, arg_parser.tapeTiming,
        arg_parser.verbose, arg_parser.targetMachine);

    // Read UEF file chunks
    if (!UEF_codec.readUefFile(arg_parser.srcFileName)) {
        cout << "Failed to read UEF File\n";
        return -1;
    }

    // Iterate over read UEF file chunks aand write them to CSW file
    ChunkInfo chunk_info;
    while (UEF_codec.processChunk(chunk_info)) {
        switch (chunk_info.chunkInfoType) {
        case ChunkInfoType::CARRIER_DUMMY:
            if (!CSW_codec.writeTone(chunk_info.data1_fp))
                return false;
            if (!CSW_codec.writeByte(0xaa, bbmDefaultDataEncoding))
                return false;
            if (!CSW_codec.writeTone((double)chunk_info.data2_i / (2 * UEF_codec.getBaseFreq())))
                return false;
            break;
        case ChunkInfoType::CARRIER:
            if (!CSW_codec.writeTone(chunk_info.data1_fp))
                return false;
            break;
        case ChunkInfoType::DATA:
            for (int i = 0; i < chunk_info.data.size(); i++) {
                if (!CSW_codec.writeByte(chunk_info.data[i], chunk_info.dataEncoding))
                    return false;
            }
            break;
        case ChunkInfoType::GAP:
            if (!CSW_codec.writeGap(chunk_info.data1_fp))
                return false;
            break;
        case ChunkInfoType::BASE_FREQ:
            if (!CSW_codec.setBaseFreq(chunk_info.data1_fp))
                return false;
            break;
        case ChunkInfoType::BAUDRATE:
            if (!CSW_codec.setBaudRate(chunk_info.data2_i))
                return false;
            break;
        case ChunkInfoType::PHASE:
            if (!CSW_codec.setPhase(chunk_info.data2_i))
                return false;
            break;
        default:
            break;
        }
    }

    if (!CSW_codec.writeSamples(arg_parser.dstFileName)) {
        cout << "Failed to write read UEF data to WAV file\n";
        return -1;
    }

    return 0;
}



