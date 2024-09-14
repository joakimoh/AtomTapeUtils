#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>

#include "MMCCodec.h"
#include "Utility.h"
#include "Debug.h"

using namespace std;

namespace fs = std::filesystem;


MMCCodec::MMCCodec(bool verbose) : mVerbose(verbose)
{

}



/*
 * Encode TAP File structure as MMC file
 */
bool MMCCodec::encode(TapeFile &tapeFile, string& filePath)
{

    if (tapeFile.blocks.empty())
        return false;


    ofstream fout(filePath, ios::out | ios::binary | ios::ate);
    if (!fout) {
        cout << "can't write to MMC file " << filePath << "\n";
        return false;
    }

    if (mVerbose)
        printf("Writing to MMC file %s\n", filePath.c_str());

    FileBlockIter ATM_block_iter;
    ATM_block_iter = tapeFile.blocks.begin();

    //
    // Write header
    //

    // Filename
    int l;
    for (l = 0; l < sizeof(ATM_block_iter->atomHdr.name) && ATM_block_iter->atomHdr.name[l] != 0; l++)
        fout.write((char*)&(ATM_block_iter->atomHdr.name[l]), 1);
    char c = 0x0;
    for (int k = 0; k < ATM_HDR_NAM_SZ - l; k++)
        fout.write((char*)&c, 1);

    // Load address
    fout.write((char*)&(ATM_block_iter->atomHdr.loadAdrLow), 1);
    fout.write((char*)&(ATM_block_iter->atomHdr.loadAdrHigh), 1);

    // Execution address
    fout.write((char*)&(ATM_block_iter->atomHdr.execAdrLow), 1);
    fout.write((char*)&(ATM_block_iter->atomHdr.execAdrHigh), 1);

    // Atom file size
    int atom_file_sz = 0;
    Byte byte_L, byte_H;
    while (ATM_block_iter < tapeFile.blocks.end()) {
        atom_file_sz += ATM_block_iter->atomHdr.lenHigh * 256 + ATM_block_iter->atomHdr.lenLow;
        ATM_block_iter++;
    }
    ATM_block_iter = tapeFile.blocks.begin(); // restore ATM block iter
    byte_L = atom_file_sz % 256;
    byte_H = atom_file_sz / 256;
    fout.write((char*)&(byte_L), 1);
    fout.write((char*)&(byte_H), 1);

    int load_addr = ATM_block_iter->atomHdr.loadAdrHigh * 256 + ATM_block_iter->atomHdr.loadAdrLow;
    int exec_addr = ATM_block_iter->atomHdr.execAdrHigh * 256 + ATM_block_iter->atomHdr.execAdrLow;
    
    string atom_filename = ATM_block_iter->atomHdr.name;

    if (mVerbose)
        tapeFile.logTAPFileHdr();


    int block_no = 0;
    atom_file_sz = 0;

    while (ATM_block_iter < tapeFile.blocks.end()) {

        //
        // Format of Acorn Atom BASIC program in memory:
        // {<cr> <linehi> <linelo> <text>} <cr> <ff>
        //
        BytesIter bi = ATM_block_iter->data.begin();
        int exec_adr = ATM_block_iter->atomHdr.execAdrHigh * 256 + ATM_block_iter->atomHdr.execAdrLow;
        int load_adr = ATM_block_iter->atomHdr.loadAdrHigh * 256 + ATM_block_iter->atomHdr.loadAdrLow;
        int data_len = ATM_block_iter->atomHdr.lenHigh * 256 + ATM_block_iter->atomHdr.lenLow;
        string name = ATM_block_iter->atomHdr.name;

        while (bi < ATM_block_iter->data.end()) {
            fout.write((char*)&(*bi++), 1);

        }

        int block_sz = ATM_block_iter->atomHdr.lenHigh * 256 + ATM_block_iter->atomHdr.lenLow;
        atom_file_sz += block_sz;
        string block_name = ATM_block_iter->atomHdr.name;
        int block_load_addr = ATM_block_iter->atomHdr.loadAdrHigh * 256 + ATM_block_iter->atomHdr.loadAdrLow;
        int block_exec_addr = ATM_block_iter->atomHdr.execAdrHigh * 256 + ATM_block_iter->atomHdr.execAdrLow;

        if (mVerbose)
           ATM_block_iter->logHdr();

        block_no++;

        ATM_block_iter++;
    }

    fout.close();

    return true;

}

/*
 * Decode TAP file as TAP File structure
 */
bool MMCCodec::decode(string& mmcFileName, TapeFile& tapeFile)
{
    ifstream fin(mmcFileName, ios::in | ios::binary | ios::ate);

    if (!fin) {
        printf("Failed top open file '%s'!\n", mmcFileName.c_str());
        return false;
    }
    filesystem::path fin_p = mmcFileName;
    string file_name = fin_p.stem().string();

    fin.seekg(0);

    // Get Atom file name
    string atom_filename = "";
    for(int i = 0; i < ATM_HDR_NAM_SZ; i++) {
        char c;
        fin.read((char*)&c, 1);
        if (c != 0x0)
            atom_filename += c;
    }

    // Get load address
    Byte byte_L, byte_H;
    fin.read((char*)&byte_L, 1);
    fin.read((char*)&byte_H, 1);
    int file_load_addr = byte_H * 256 + byte_L;
 
    // Get execution address
    fin.read((char*)&byte_L, 1);
    fin.read((char*)&byte_H, 1);
    int file_exec_addr = byte_H * 256 + byte_L;

    // Get Length
    fin.read((char*)&byte_L, 1);
    fin.read((char*)&byte_H, 1);
    int atom_file_len = byte_H * 256 + byte_L;

    if (mVerbose)
        printf("%s %.4x %4.x %.4x\n", atom_filename.c_str(), file_load_addr, file_exec_addr, atom_file_len);

    Bytes data;
    // Get data
    char c;
    while (fin.read((char*) &c,1)) {
        data.push_back(c);
    }


    //
    // Create TAP file structure
    //
    FileBlock block(ACORN_ATOM);
    tapeFile.blocks.clear();
    tapeFile.complete = true;
    tapeFile.validFileName = block.filenameFromBlockName(atom_filename);
    tapeFile.isBasicProgram = true;

    BytesIter data_iterator = data.begin();


    // Create ATM block  
    int block_no = 0;
    bool new_block = true;
    int count = 0;
    BytesIter data_iter = data.begin();
    int block_sz;
    int load_address = file_load_addr;
    while (data_iter < data.end()) {

        if (new_block) {
            count = 0;
            if (data.end() - data_iter < 256)
                block_sz = (int) (data.end() - data_iter);
            else
                block_sz = 256;
            block.data.clear();
            block.tapeStartTime = -1;
            block.tapeEndTime = -1;
            block.atomHdr.execAdrHigh = file_exec_addr / 256;
            block.atomHdr.execAdrLow = file_exec_addr % 256;
            block.atomHdr.loadAdrHigh = load_address / 256;
            block.atomHdr.loadAdrLow = load_address % 256;
            for (int i = 0; i < sizeof(block.atomHdr.name); i++) {
                if (i < atom_filename.size())
                    block.atomHdr.name[i] = atom_filename[i];
                else
                    block.atomHdr.name[i] = 0;
            }
            new_block = false;
        }


        if (count < block_sz) {
            block.data.push_back(*data_iter++);
            count++;
        }
        else {
            block.atomHdr.lenHigh = 1;
            block.atomHdr.lenLow = 0;
            tapeFile.blocks.push_back(block);
            new_block = true;
            load_address += count;
        }




    }

    // Add last block
    block.atomHdr.lenHigh = count / 256;
    block.atomHdr.lenLow = count % 256;
    tapeFile.blocks.push_back(block);

   

    return true;
}
