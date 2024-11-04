#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>
#include <cstdint>

#include "TAPCodec.h"
#include "Utility.h"
#include "Logging.h"
#include <string.h>
#include "BinCodec.h"

using namespace std;

namespace fs = std::filesystem;

bool TAPCodec::openTapeFile(string& filePath)
{
    mTAPFiles.clear();

    mTapeFile_p = new ofstream(filePath, ios::out | ios::binary | ios::ate);
    if (!*mTapeFile_p) {
        cout << "can't write to TAP file " << filePath << "\n";
        return false;
    }

    return true;
}

bool TAPCodec::closeTapeFile()
{

    mTapeFile_p->close();

    delete mTapeFile_p;

    return true;
}


TAPCodec::TAPCodec(Logging logging, TargetMachine targetMachine) : mDebugInfo(logging), mTargetMachine(targetMachine)
{
}

/*
 * Encode TAP File structure as TAP file
 */
bool TAPCodec::encode(TapeFile& tapeFile, string& filePath)
{
    if (!openTapeFile(filePath)) {
        return false;
    }

    encode(tapeFile);

    if (!closeTapeFile()) {
        return false;
    }

    return true;
}
/*
     * Encode TAP File structure into already open TAP file
     */
bool TAPCodec::encode(TapeFile& tapeFile)
{

    if (tapeFile.blocks.empty())
        return false;

    FileBlockIter block_iter = tapeFile.blocks.begin();
    unsigned block_no = 0;

    if (mDebugInfo.verbose) {
        cout << "\n";
        tapeFile.logFileHdr();
    }

    
    // Get file data for header (exec adr, load adr & file sz)
    unsigned exec_adr = tapeFile.header.execAdr;
    unsigned load_adr = tapeFile.header.loadAdr;
    unsigned file_sz = tapeFile.header.size;
    string program_name = tapeFile.header.name;

    if (mDebugInfo.verbose)
        cout << "\nEncoding program '" << program_name << "' as a TAP file...\n\n";

    // Write TAP header
    *mTapeFile_p << setw(16) << setfill('\x0') << left << program_name; //  zero-padded program name
    *mTapeFile_p << (char)(exec_adr & 0xff);    // execution address - little endian
    *mTapeFile_p << (char) ((exec_adr >> 8) & 0xff); 
    *mTapeFile_p << (char) (load_adr & 0xff);   // load address - little endian
    *mTapeFile_p << (char)((load_adr >> 8) & 0xff); 
    *mTapeFile_p << (char) (file_sz & 0xff);    // file size - little endian
     *mTapeFile_p << (char)((file_sz >> 8) & 0xff); 

    // Write TAP blocks
    while (block_iter < tapeFile.blocks.end()) {

        // Write data
        if (block_iter->data.size() > 0)
            mTapeFile_p->write((char*)&block_iter->data[0], block_iter->data.size());

        if (mDebugInfo.verbose)
            block_iter->logFileBlockHdr();

        block_iter++;
        block_no++;
    }

    if (mDebugInfo.verbose) {
        cout << "\n";
        tapeFile.logFileHdr();
        cout << "\nDone encoding program '" << program_name << "' as a TAP file...\n\n";
    }

    return true;

}

/*
 * Decode TAP file as TAP File structure
 */
bool TAPCodec::decode(string& tapFileName, TapeFile& tapeFile)
{
    ifstream fin(tapFileName, ios::in | ios::binary | ios::ate);

    if (!fin) {
        printf("Failed to open file '%s'!\n", tapFileName.c_str());
        return false;
    }
    filesystem::path fin_p = tapFileName;
    string file_name = fin_p.stem().string();

    // Get file size
    fin.seekg(0, ios::end);
    streamsize file_size = fin.tellg();

    // Start reading from the beginning of the file
    fin.seekg(0);

    if (mDebugInfo.verbose)
        cout << "\nDecoding TAP file '" << tapFileName << "'...\n\n";

    // Read one program from the TAP file
    if (!decodeSingleFile(fin, (int) file_size, tapeFile)) {
        cout << "Failed to decode TAP file '" << tapFileName << "'!\n";
    }

    fin.close();

    if (mDebugInfo.verbose)
        cout << "\nDone decoding TAP file '" << tapFileName << "'...\n\n";

    return true;
}

bool TAPCodec::decodeMultipleFiles(string& tapFileName, vector<TapeFile> &tapFiles)
{
    ifstream fin(tapFileName, ios::in | ios::binary | ios::ate);

    if (!fin) {
        printf("Failed to open file '%s'!\n", tapFileName.c_str());
        return false;
    }
    filesystem::path fin_p = tapFileName;
    string file_name = fin_p.stem().string();

    // Get file size
    fin.seekg(0, ios::end);
    streamsize file_size = fin.tellg();

    // Start reading from the beginning of the file
    fin.seekg(0);

    if (mDebugInfo.verbose)
        cout << "\nDecoding TAP file '" << tapFileName << "'...\n\n";

    // Read one program from the TAP file
    TapeFile TAP_file(mTargetMachine);
    while (decodeSingleFile(fin, file_size, TAP_file)) {
        tapFiles.push_back(TAP_file);
    }

    fin.close();

    if (mDebugInfo.verbose)
        cout << "\nDone decoding TAP file '" << tapFileName << "'...\n\n";

    return true;
}

bool TAPCodec::decodeSingleFile(ifstream &fin, streamsize file_size, TapeFile &tapeFile)
{

    // Test for end of file
    if (fin.tellg() == file_size)
        return false;

    // Read TAP header to get name and size etc.
    string program_name;
    unsigned file_sz, exec_adr, file_load_adr;
    if (fin.tellg() < (streampos) (file_size - sizeof(ATMHdr))) {

        // Program name
        Byte program_name_bytes[16], exec_adr_bytes[2], load_adr_bytes[2], file_sz_bytes[2];
        if (!Utility::readBytes(fin, program_name_bytes, 16))
            return false;
        for (int i = 0; i < 16 && program_name_bytes[i] != 0x0; program_name = program_name + (char)program_name_bytes[i++]);

        // Execution address
        if (!Utility::readBytes(fin, exec_adr_bytes, 2))
            return false;
        exec_adr = exec_adr_bytes[1] * 256 + exec_adr_bytes[0];

        // Load address
        if (!Utility::readBytes(fin, load_adr_bytes, 2))
            return false;
        file_load_adr = load_adr_bytes[1] * 256 + load_adr_bytes[0];

        // File size
        if (!Utility::readBytes(fin, file_sz_bytes, 2))
            return false;
        file_sz = file_sz_bytes[1] * 256 + file_sz_bytes[0];

        tapeFile.firstBlock = 0;
        tapeFile.blocks.clear();
        tapeFile.complete = true;
        tapeFile.baudRate = (mTargetMachine==ACORN_ATOM?300:1200);
        FileHeader file_header(program_name, file_load_adr, exec_adr, file_sz, mTargetMachine);
        tapeFile.header = file_header;
        if (mDebugInfo.verbose)
            tapeFile.logFileHdr();
    }
    else {
        cout << "Invalid TAP header detected!\n";
        return false;
    }

    // Create the blocks 
    unsigned block_no = 0;
    bool done = false;
    unsigned expected_block_sz = (file_sz >= 256 ? 256 : file_sz);
    unsigned block_load_adr = file_load_adr;
    unsigned read_bytes = 0;
    while (!done) {

        
        if (fin.tellg() <= file_size - expected_block_sz && read_bytes < file_sz) {

            // Create block and initialise its header with tape file header data
            uint32_t load_adr = (mTargetMachine == ACORN_ATOM ? block_load_adr : file_load_adr);
            FileBlock block(mTargetMachine, program_name, block_no, load_adr, exec_adr, expected_block_sz);

            if (mDebugInfo.verbose)
                block.logFileBlockHdr();

            // Read block data
            block.data.resize(expected_block_sz);
            fin.read((char*)&block.data[0], expected_block_sz);
            read_bytes += expected_block_sz;

            tapeFile.blocks.push_back(block);

            tapeFile.lastBlock = block_no;

            block_no++;

            block_load_adr += expected_block_sz;

            if (read_bytes < file_sz - 256)
                expected_block_sz = 256;
            else
                expected_block_sz = file_sz - read_bytes;
        }
        else if (fin.tellg() < file_size && mDebugInfo.verbose) {
            cout << "Warning the TAP file contains " << file_size - fin.tellg() << " extra bytes at the end of the file!\n";
            done = true;
        }
        else
            done = true;
    }

    // Set the type for each block (FIRST, LAST, OTHER or SINGLE) - can only be made when the no of blocks are known
    if (!tapeFile.setBlockTypes())
        return false;

    if (mDebugInfo.verbose) {
        cout << "\n";
        tapeFile.logFileHdr();
        cout << "\nDone decoding TAP file...\n";
    }


    return true;
}