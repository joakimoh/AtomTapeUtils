#include "BinCodec.h"
#include "Utility.h"

//
// Create BIN Codec.
//
BinCodec::BinCodec(Logging logging): mDebugInfo(logging)
{
	
}

//
// Encode TAP File structure as binary data
//
bool BinCodec::encode(TapeFile& tapeFile, FileMetaData &fileMetaData, Bytes& data)
{
    if (tapeFile.blocks.empty()) {
        cout << "Empty TAP file => no DATA to encode!\n";
        return false;
    }

    if (mDebugInfo.verbose)
        cout << "\nEncoding program '" << tapeFile.programName << "' as a BINARY data...\n\n";

    // Extract meta data
    fileMetaData.execAdr = tapeFile.blocks[0].execAdr();
    fileMetaData.loadAdr = tapeFile.blocks[0].loadAdr();
    fileMetaData.name = tapeFile.programName;
    fileMetaData.targetMachine = tapeFile.metaData.targetMachine;

    // Extract data
    FileBlockIter file_block_iter = tapeFile.blocks.begin();
    while (file_block_iter < tapeFile.blocks.end()) {
        BytesIter bi = file_block_iter->data.begin();
        while (bi < file_block_iter->data.end()) {
            data.push_back(*bi++);
        }
        file_block_iter++;
    }

    if (mDebugInfo.verbose) {
        cout << "\n";
        tapeFile.logTAPFileHdr();
        cout << "\nDone encoding program '" << tapeFile.blocks[0].bbmHdr.name << "' as binary data...\n\n";
    }
	return true;
}

//
// Decode binary data as Tape File structure
//
bool BinCodec::decode(FileMetaData fileMetaData, Bytes &data, TapeFile& tapeFile)
{
    if (data.size() == 0)
        return false;

    FileBlock block(fileMetaData.targetMachine);

    string block_name = fileMetaData.name;

    tapeFile.init();
    tapeFile.complete = true;
    tapeFile.programName = fileMetaData.name;
    tapeFile.metaData = fileMetaData;


    int load_adr = fileMetaData.loadAdr;
    int block_no = 0;
    bool new_block = true;
    int count = 0;
    BytesIter data_iter = data.begin();
    int block_sz;
    int tape_file_sz = 0;

    while (data_iter < data.end()) {

        if (new_block) {
            count = 0;
            if (data.end() - data_iter < 256)
                block_sz = (int)(data.end() - data_iter);
            else
                block_sz = 256;
            tape_file_sz += block_sz;

            if (!block.init())
                return false;

            block.targetMachine = fileMetaData.targetMachine;

            if (!block.encodeTAPHdr(block_name, fileMetaData.loadAdr, load_adr, fileMetaData.execAdr, block_no, block_sz)) {
                cout << "Failed to encode Tape File header for program '" << block_name << "'!\n";
                return false;
            }
            new_block = false;

        }

        if (count < block_sz) {
            block.data.push_back(*data_iter++);
            count++;
        }
        if (count == block_sz) {

            if (mDebugInfo.verbose)
                block.logHdr();

            tapeFile.blocks.push_back(block);
            new_block = true;

            load_adr += count;
            block_no++;
         }


    }

    // Set the type for each block (FIRST, LAST, OTHER or SINGLE) - can only be made when the no of blocks are known
    if (!tapeFile.setBlockTypes())
        return false;


    if (mDebugInfo.verbose) {
        cout << "\n";
        tapeFile.logTAPFileHdr();
    }

    if (mDebugInfo.verbose)
        cout << "\nDone decoding program '" << block_name << "'...\n\n";


	return true;
}