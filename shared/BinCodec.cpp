#include "BinCodec.h"
#include "Utility.h"
#include <iostream>
#include <fstream>
#include <filesystem>

//
// Create BIN Codec.
//
BinCodec::BinCodec(Logging logging): mDebugInfo(logging)
{
	
}

//
// Encode TAP File structure as binary data
//
bool BinCodec::encode(TapeFile& tapeFile, FileHeader &fileMetaData, Bytes& data)
{
    if (tapeFile.blocks.empty()) {
        cout << "Empty TAP file => no DATA to encode!\n";
        return false;
    }

    if (mDebugInfo.verbose)
        cout << "\nEncoding program '" << tapeFile.header.name << "' as a BINARY data...\n\n";

    // Extract meta data
    fileMetaData.execAdr = tapeFile.header.execAdr;
    fileMetaData.loadAdr = tapeFile.header.loadAdr;
    fileMetaData.name = tapeFile.header.name;
    fileMetaData.targetMachine = tapeFile.header.targetMachine;

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
        tapeFile.logFileHdr();
        cout << "\nDone encoding program '" << tapeFile.header.name << "' as binary data...\n\n";
    }
	return true;
}

/*
 * Encode TAP File structure as Binary data
 */
bool BinCodec::encode(TapeFile& tapeFile, string& binFileName)
{

    if (tapeFile.blocks.size() == 0)
        return false;

    FileBlockIter file_block_iter = tapeFile.blocks.begin();
    int sz = 0;
    for (; file_block_iter < tapeFile.blocks.end(); sz += (int)(*file_block_iter++).data.size());
    if (sz == 0)
        return false;

    // Create the output file
    ofstream fout(binFileName, ios::out | ios::binary | ios::ate);
    if (!fout) {
        cout << "can't write to file " << binFileName << "\n";
        return false;
    }

    // Write all block data to the file
    file_block_iter = tapeFile.blocks.begin();
    while (file_block_iter < tapeFile.blocks.end()) {
        BytesIter data_iter = file_block_iter->data.begin();
        while (data_iter < file_block_iter->data.end())
            fout << *data_iter++;
        file_block_iter++;
    }
    fout.close();

    return true;
}

//
// Decode binary data as Tape File structure
//
bool BinCodec::decode(FileHeader fileMetaData, Bytes &data, TapeFile& tapeFile)
{
    if (data.size() == 0)
        return false;

    FileBlock block(fileMetaData.targetMachine);

    string block_name = fileMetaData.name;

    tapeFile.init();
    tapeFile.complete = true;
    tapeFile.header.name = fileMetaData.name;
    tapeFile.header = fileMetaData;


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
                block.logFileBlockHdr();

            // Mirror the complete file's locked status to each block
            block.locked = tapeFile.header.locked;

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
        tapeFile.logFileHdr();
    }

    if (mDebugInfo.verbose)
        cout << "\nDone decoding program '" << block_name << "'...\n\n";


	return true;
}

bool BinCodec::generateInfFile(string dir, TapeFile& tapeFile)
{
    if (tapeFile.blocks.size() == 0)
        return false;

    // Create INF file
    string INF_file_name = Utility::crEncodedFileNamefromDir(dir, tapeFile, "inf");
    ofstream INF_file(INF_file_name, ios::out);

    if (!INF_file)
        return false;

    uint32_t load_adr = tapeFile.blocks[0].loadAdr;
    uint32_t exec_adr = tapeFile.blocks[0].execAdr;
    if ((load_adr & 0x20000) != 0)
        load_adr |= 0xff0000;
    if ((exec_adr & 0x20000) != 0)
        exec_adr |= 0xff0000;
    INF_file << setw(9) << left << tapeFile.blocks[0].name << right << hex << " " <<
        setw(6) << setfill('0') << load_adr << " " << setw(6) << setfill('0') << exec_adr;
    INF_file.close();

    return true;
}