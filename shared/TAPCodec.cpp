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


TAPCodec::TAPCodec(TAPFile &tapFile, bool verbose): mTapFile(tapFile), mVerbose(verbose)
{

}



/*
 * Encode TAP File structure as TAP file
 */
bool TAPCodec::encode(string& filePath)
{

    if (mTapFile.blocks.empty())
        return false;


    ofstream fout(filePath, ios::out | ios::binary | ios::ate);
    if (!fout) {
        cout << "can't write to TAP file " << filePath << "\n";
        return false;
    }

    ATMBlockIter ATM_block_iter = mTapFile.blocks.begin();
    unsigned block_no = 0;

    if (mVerbose)
        cout << "\nEncoding TAP file '" << filePath << "' consisting of " << mTapFile.blocks.size() << " blocks...\n";

    // Get atom file data for header (exec adr, load adr & file sz)
    unsigned exec_adr = mTapFile.blocks[0].hdr.execAdrHigh * 256 + mTapFile.blocks[0].hdr.execAdrLow;
    unsigned load_adr = mTapFile.blocks[0].hdr.loadAdrHigh * 256 + mTapFile.blocks[0].hdr.loadAdrLow;
    unsigned atom_file_sz = 0;
    string atom_filename = mTapFile.blocks[0].hdr.name;
    for (int i = 0; i < mTapFile.blocks.size(); i++) {
        atom_file_sz += mTapFile.blocks[i].hdr.lenHigh * 256 + mTapFile.blocks[i].hdr.lenLow;
    }

    // Write TAP header
    ATMBlock block;
    block.hdr.execAdrHigh = (exec_adr >> 8) & 0xff;
    block.hdr.execAdrLow = exec_adr & 0xff;
    block.hdr.loadAdrHigh = (load_adr >> 8) & 0xff;
    block.hdr.loadAdrLow = load_adr & 0xff;
    block.hdr.lenHigh = (atom_file_sz >> 8) & 0xff;
    block.hdr.lenLow = atom_file_sz & 0xff;
    strncpy(block.hdr.name, atom_filename.c_str(), ATM_MMC_HDR_NAM_SZ);
    fout.write((char*)&block.hdr, sizeof(block.hdr));

    // Write TAP ATM blocks
    while (ATM_block_iter < mTapFile.blocks.end()) {

        // Write data
        fout.write((char*)&ATM_block_iter->data[0], ATM_block_iter->data.size());

        if (mVerbose) {
            string atom_filename = ATM_block_iter->hdr.name;
            unsigned data_sz = ATM_block_iter->hdr.lenHigh * 256 + ATM_block_iter->hdr.lenLow;
            unsigned load_adr_start = ATM_block_iter->hdr.loadAdrHigh * 256 + ATM_block_iter->hdr.loadAdrLow;
            unsigned exec_adr_start = ATM_block_iter->hdr.execAdrHigh * 256 + ATM_block_iter->hdr.execAdrLow;
            unsigned load_adr_end = load_adr_start + data_sz - 1;

            printf("\n%15s %4x %4x %4x %2d %5d\n", atom_filename.c_str(),
                load_adr_start, load_adr_end, exec_adr_start, block_no + 1, data_sz
            );

        }
        ATM_block_iter++;
        block_no++;
    }


    fout.close();

    if (mVerbose) {

        if (mVerbose) {
            printf("\n%15s %4x %4x %4x %2d %5d\n", atom_filename.c_str(),
                load_adr, load_adr + atom_file_sz - 1, exec_adr, block_no, atom_file_sz
            );
        }

        cout << "Done encoding TAP file...\n\n";
    }

    return true;

}

/*
 * Decode TAP file as TAP File structure
 */
bool TAPCodec::decode(string& tapFileName)
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
        cout << "\nDecoding TAP file '" << tapFileName << "' of size " << file_size << " bytes...\n\n";

    // Read one Atom File from the TAP file
    if (!decodeSingleFile(fin, file_size, mTapFile)) {
        cout << "Failed to decode TAP file '" << tapFileName << "'!\n";
    }

    fin.close();

    return true;
}

bool TAPCodec::decodeMultipleFiles(string& tapFileName, vector<TAPFile> &atomFiles)
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
        cout << "\nDecoding TAP file '" << tapFileName << "' of size " << file_size << " bytes...\n\n";

    // Read one Atom File from the TAP file
    TAPFile TAP_file;
    while (decodeSingleFile(fin, file_size, TAP_file)) {
        atomFiles.push_back(TAP_file);
    }

    fin.close();

    return true;
}

bool TAPCodec::decodeSingleFile(ifstream &fin, unsigned file_size, TAPFile &tapFile)
{
    // Read TAP header to get name and size etc
    string atom_filename;
    unsigned atom_file_sz, exec_adr, load_adr;
    if (fin.tellg() < file_size - sizeof(ATMHdr)) {
        ATMBlock block;
        fin.read((char*)&block.hdr, sizeof(block.hdr));
        exec_adr = block.hdr.execAdrHigh * 256 + block.hdr.execAdrLow;
        load_adr = block.hdr.loadAdrHigh * 256 + block.hdr.loadAdrLow;
        atom_file_sz = block.hdr.lenHigh * 256 + block.hdr.lenLow;
        atom_filename = block.hdr.name;
        tapFile.firstBlock = 0;
        tapFile.validFileName = filenameFromBlockName(atom_filename);
        tapFile.blocks.clear();
        tapFile.complete = true;
        tapFile.isAbcProgram = true;
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

        ATMBlock block;
        if (fin.tellg() <= file_size - expected_block_sz && read_bytes < atom_file_sz) {

            if (mVerbose) {
                printf("%15s %4x %4x %4x %2d %5d\n", atom_filename.c_str(),
                    block_load_adr, block_load_adr + expected_block_sz - 1, exec_adr, block_no + 1, expected_block_sz
                );
            }

            // Fill block hdr based on TAP header data
            block.hdr.execAdrHigh = (exec_adr >> 8) & 0xff;
            block.hdr.execAdrLow = exec_adr & 0xff;
            block.hdr.loadAdrHigh = (block_load_adr >> 8) & 0xff;
            block.hdr.loadAdrLow = block_load_adr & 0xff;
            block.hdr.lenHigh = (expected_block_sz >> 8) & 0xff;
            block.hdr.lenLow = expected_block_sz & 0xff;
            strncpy(block.hdr.name, atom_filename.c_str(), ATM_MMC_HDR_NAM_SZ);


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
        printf("\n%15s %4x %4x %4x %2d %5d\n", atom_filename.c_str(),
            load_adr, load_adr + atom_file_sz - 1, exec_adr, block_no, atom_file_sz
        );
        cout << "\nDone decoding TAP file...\n";
    }


    return true;
}

/*
 * Get the codec's TAP file
 */
bool TAPCodec::getTAPFile(TAPFile& tapFile)
{
    tapFile = mTapFile;

    return true;
}



/*
 * Reinitialise codec with a new TAP file
 */
bool TAPCodec::setTAPFile(TAPFile& tapFile)
{
    mTapFile = tapFile;

    return true;

}
