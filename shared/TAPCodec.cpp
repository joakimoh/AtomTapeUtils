#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>

#include "TAPCodec.h"
#include "Utility.h"
#include "Debug.h"
#include <string.h>

using namespace std;

namespace fs = std::filesystem;


TAPCodec::TAPCodec(bool verbose) : mVerbose(verbose)
{

}
bool TAPCodec::bytes2TAP(Bytes& data, TargetMachine targetMachine, string tapeFileName, uint32_t fileLoadAdr, uint32_t execAdr, TapeFile& tapeFile)
{

    if (targetMachine) {
        tapeFile.fileType = BBC_MODEL_B;
        tapeFile.baudRate = 1200;
    }
    else {
        tapeFile.fileType = ACORN_ATOM;
        tapeFile.baudRate = 300;
    }

    tapeFile.isBasicProgram = false;
    tapeFile.complete = true;
    tapeFile.validFileName = FileBlock::filenameFromBlockName(targetMachine, tapeFileName);
    tapeFile.validTiming = false;
    

    BytesIter data_iter = data.begin();
    int count = 0;
    uint32_t load_adr = fileLoadAdr;
    uint32_t block_no = 0;
    FileBlock block(ACORN_ATOM);
    uint32_t block_sz;
    while (data_iter < data.end()) {
        if (count == 0) { // new block        
            block_sz = 256;
            if (data.end() - data_iter < 256)
                block_sz = (int) (data.end() - data_iter);
            if (!block.init())
                return false;
            if (!block.encodeTAPHdr(tapeFileName, fileLoadAdr, load_adr, execAdr, block_no, block_sz)) {
                cout << "Failed to create Tape Block for Tape File " << tapeFileName << "\n";
                return false;
            }
            tapeFile.blocks.push_back(block);
        }
        else {
            block.data.push_back(*data_iter++);
            count++;
        }
        if (count == block_sz) {
            tapeFile.blocks.push_back(block);
            count = 0;
        }
    }

    return true;
}


bool TAPCodec::tap2Bytes(TapeFile& tapeFile, uint32_t& loadAdress, Bytes& data)
{
    FileBlockIter block_iter = tapeFile.blocks.begin();

    if (tapeFile.blocks.size() == 0)
        return false;

    if (tapeFile.fileType == ACORN_ATOM)
        loadAdress = block_iter->atomHdr.execAdrHigh * 256 + block_iter->atomHdr.execAdrLow;
    else
        loadAdress = Utility::bytes2uint(&block_iter->bbmHdr.loadAdr[0], 4, true);

    while (block_iter < tapeFile.blocks.end()) {
        Bytes &block = block_iter->data;
        BytesIter data_iter = block.begin();
        while (data_iter < block.end())
            data.push_back(*data_iter++);
        block_iter++;
    }
    return true;
}


/*
 * Encode TAP File structure as TAP file
 */
bool TAPCodec::encode(TapeFile &tapeFile, string& filePath)
{

    if (tapeFile.blocks.empty())
        return false;


    ofstream fout(filePath, ios::out | ios::binary | ios::ate);
    if (!fout) {
        cout << "can't write to TAP file " << filePath << "\n";
        return false;
    }

    FileBlockIter ATM_block_iter = tapeFile.blocks.begin();
    unsigned block_no = 0;

    if (mVerbose)
        tapeFile.logTAPFileHdr();

    if (mVerbose)
        cout << "\nEncoding program '" << tapeFile.blocks[0].atomHdr.name << "' as a TAP file...\n\n";

    // Get atom file data for header (exec adr, load adr & file sz)
    unsigned exec_adr = tapeFile.blocks[0].atomHdr.execAdrHigh * 256 + tapeFile.blocks[0].atomHdr.execAdrLow;
    unsigned load_adr = tapeFile.blocks[0].atomHdr.loadAdrHigh * 256 + tapeFile.blocks[0].atomHdr.loadAdrLow;
    unsigned atom_file_sz = 0;
    string atom_filename = tapeFile.blocks[0].atomHdr.name;
    for (int i = 0; i < tapeFile.blocks.size(); i++) {
        atom_file_sz += tapeFile.blocks[i].atomHdr.lenHigh * 256 + tapeFile.blocks[i].atomHdr.lenLow;
    }

    // Write TAP header
    FileBlock block(ACORN_ATOM);
    block.atomHdr.execAdrHigh = (exec_adr >> 8) & 0xff;
    block.atomHdr.execAdrLow = exec_adr & 0xff;
    block.atomHdr.loadAdrHigh = (load_adr >> 8) & 0xff;
    block.atomHdr.loadAdrLow = load_adr & 0xff;
    block.atomHdr.lenHigh = (atom_file_sz >> 8) & 0xff;
    block.atomHdr.lenLow = atom_file_sz & 0xff;
    strncpy(block.atomHdr.name, atom_filename.c_str(), ATM_HDR_NAM_SZ);
    fout.write((char*)&block.atomHdr, sizeof(block.atomHdr));

    // Write TAP ATM blocks
    while (ATM_block_iter < tapeFile.blocks.end()) {

        // Write data
        if (ATM_block_iter->data.size() > 0)
            fout.write((char*)&ATM_block_iter->data[0], ATM_block_iter->data.size());

        if (mVerbose)
            ATM_block_iter->logHdr();

        ATM_block_iter++;
        block_no++;
    }


    fout.close();

    if (mVerbose) {
        tapeFile.logTAPFileHdr();
        cout << "\nDone encoding program '" << tapeFile.blocks[0].atomHdr.name << "' as a TAP file...\n\n";
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

    if (mVerbose)
        cout << "\nDecoding TAP file '" << tapFileName << "'...\n\n";

    // Read one Atom File from the TAP file
    if (!decodeSingleFile(fin, (int) file_size, tapeFile)) {
        cout << "Failed to decode TAP file '" << tapFileName << "'!\n";
    }

    fin.close();

    if (mVerbose)
        cout << "\nDone decoding TAP file '" << tapFileName << "'...\n\n";

    return true;
}

bool TAPCodec::decodeMultipleFiles(string& tapFileName, vector<TapeFile> &atomFiles)
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

    if (mVerbose)
        cout << "\nDecoding TAP file '" << tapFileName << "'...\n\n";

    // Read one Atom File from the TAP file
    TapeFile TAP_file(ACORN_ATOM);
    while (decodeSingleFile(fin, file_size, TAP_file)) {
        atomFiles.push_back(TAP_file);
    }

    fin.close();

    if (mVerbose)
        cout << "\nDone decoding TAP file '" << tapFileName << "'...\n\n";

    return true;
}

bool TAPCodec::decodeSingleFile(ifstream &fin, streamsize file_size, TapeFile &tapFile)
{

    // Test for end of file
    if (fin.tellg() == file_size)
        return false;

    // Read TAP header to get name and size etc
    string atom_filename;
    unsigned atom_file_sz, exec_adr, load_adr;
    if (fin.tellg() < (streampos) (file_size - sizeof(ATMHdr))) {
        FileBlock block(ACORN_ATOM);
        fin.read((char*)&block.atomHdr, sizeof(block.atomHdr));
        exec_adr = block.atomHdr.execAdrHigh * 256 + block.atomHdr.execAdrLow;
        load_adr = block.atomHdr.loadAdrHigh * 256 + block.atomHdr.loadAdrLow;
        atom_file_sz = block.atomHdr.lenHigh * 256 + block.atomHdr.lenLow;
        atom_filename = block.atomHdr.name;
        tapFile.firstBlock = 0;
        tapFile.validFileName = block.filenameFromBlockName(atom_filename);
        tapFile.blocks.clear();
        tapFile.complete = true;
        tapFile.isBasicProgram = true;
        tapFile.baudRate = 300;
    }
    else {
        cout << "Invalid TAP header detected!\n";
        return false;
    }

    // Create the ATM blocks 
    unsigned block_no = 0;
    bool done = false;
    unsigned expected_block_sz = (atom_file_sz >= 256 ? 256 : atom_file_sz);
    unsigned block_load_adr = load_adr;
    unsigned read_bytes = 0;
    while (!done) {

        FileBlock block(ACORN_ATOM);
        if (fin.tellg() <= file_size - expected_block_sz && read_bytes < atom_file_sz) {

            

            // Fill block atomHdr based on TAP header data
            block.atomHdr.execAdrHigh = (exec_adr >> 8) & 0xff;
            block.atomHdr.execAdrLow = exec_adr & 0xff;
            block.atomHdr.loadAdrHigh = (block_load_adr >> 8) & 0xff;
            block.atomHdr.loadAdrLow = block_load_adr & 0xff;
            block.atomHdr.lenHigh = (expected_block_sz >> 8) & 0xff;
            block.atomHdr.lenLow = expected_block_sz & 0xff;
            strncpy(block.atomHdr.name, atom_filename.c_str(), ATM_HDR_NAM_SZ);

            if (mVerbose)
                block.logHdr();
 

            // Read block data
            block.data.resize(expected_block_sz);
            fin.read((char*)&block.data[0], expected_block_sz);
            read_bytes += expected_block_sz;

            tapFile.blocks.push_back(block);

            tapFile.lastBlock = block_no;

            block_no++;

            block_load_adr += expected_block_sz;

            if (read_bytes < atom_file_sz - 256)
                expected_block_sz = 256;
            else
                expected_block_sz = atom_file_sz - read_bytes;
        }
        else if (fin.tellg() < file_size && mVerbose) {
            cout << "Warning the TAP file contains " << file_size - fin.tellg() << " extra bytes at the end of the file!\n";
            done = true;
        }
        else
            done = true;
    }

    if (mVerbose) {
        tapFile.logTAPFileHdr();
         cout << "\nDone decoding TAP file...\n";
    }


    return true;
}





/*
 * Decode TAP File structure and store as Binary data
 */
bool TAPCodec::data2Binary(TapeFile& tapeFile, string& binFileName)
{

    if (tapeFile.blocks.size() == 0)
        return false;

    FileBlockIter file_block_iter = tapeFile.blocks.begin();
    int sz = 0;
    for (; file_block_iter < tapeFile.blocks.end(); sz += (int) (*file_block_iter++).data.size());
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