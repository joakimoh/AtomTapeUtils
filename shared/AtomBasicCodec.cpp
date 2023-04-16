#include "AtomBasicCodec.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Utility.h"
#include "Debug.h"

using namespace std;


AtomBasicCodec::AtomBasicCodec(TAPFile& tapFile) : mTapFile(tapFile)
{
}

AtomBasicCodec::AtomBasicCodec()
{
}



bool AtomBasicCodec::encode(string & filePath)
{

    if (mTapFile.blocks.empty()) {
        DBG_PRINT(DBG, "TAP File is empty => no Program file created!\n");
        return false;
    }

    ATMBlockIter ATM_block_iter = mTapFile.blocks.begin();

    if (!(mTapFile.isAbcProgram)) {
        DBG_PRINT(ERR, "File '%s' likely doesn\'t contain an Acorn Atom Basic Program => ABC file generated could be unreadable...\n", ATM_block_iter->hdr.name);
    }



    ofstream fout(filePath);
    if (!fout) {
        DBG_PRINT(ERR, "Can't write to program file '%s'!\n", filePath.c_str());
        return false;
    }

    DBG_PRINT(DBG, "Writing to program file '%s'!\n", filePath.c_str());

    int line_pos = -1;
    int line_no_high = 0;
    bool first_line = true;
    bool end_of_program = false;

    while (ATM_block_iter < mTapFile.blocks.end()) {

        //
        // Format of Acorn Atom BASIC program in memory:
        // {<cr> <linehi> <linelo> <text>} <cr> <ff>
        //
        BytesIter bi = ATM_block_iter->data.begin();
        int exec_adr = ATM_block_iter->hdr.execAdrHigh * 256 + ATM_block_iter->hdr.execAdrLow;
        int load_adr = ATM_block_iter->hdr.loadAdrHigh * 256 + ATM_block_iter->hdr.loadAdrLow;
        int data_len = ATM_block_iter->hdr.lenHigh * 256 + ATM_block_iter->hdr.lenLow;
        string name = ATM_block_iter->hdr.name;

        while (bi < ATM_block_iter->data.end() && !end_of_program) {
            if (line_pos == 0) {
                if (*bi == 0xff) {
                    bi++;
                    end_of_program = true;
                    int n_non_ABC_bytes = ATM_block_iter->data.end() - bi;
                    if (ATM_block_iter->data.end() - bi > 0) {
                        DBG_PRINT(ERR, "Program file '%s' contains %d extra bytes after end of program - skipping this data!\n", filePath.c_str(), n_non_ABC_bytes);
                    }
                }
                else {
                    line_no_high = int(*bi++);
                    line_pos++;
                }
            }
            else if (line_pos == 1) {
                int line_no_low = int(*bi++);
                line_pos = -1;
                int line_no = line_no_high * 256 + line_no_low;
                char line_no_s[7];
                sprintf_s(line_no_s, "%5d", line_no);
                fout << line_no_s;
            }
            else if (*bi == 0xd) {
                line_pos = 0;
                bi++;
                if (!first_line) {
                    fout << "\n";
                }
                first_line = false;

            } else {
                fout << (char)*bi++;
            }

        }

        DBG_PRINT(DBG, "%s %.4x %.4x %.3x\n", name.c_str(), load_adr, exec_adr, data_len);

        ATM_block_iter++;
    }

    if (!end_of_program)
        DBG_PRINT(DBG, "Program '%s' didn't terminate with 0xff!\n", mTapFile.blocks.front().hdr.name);
 
    DBG_PRINT(DBG, "Program file '%s' created from %d blocks!\n", filePath.c_str(), mTapFile.blocks.size());

    fout.close();

    return true;
}

bool AtomBasicCodec::decode(string &programFileName)
{

    ifstream fin(programFileName);

    if (!fin) {
        DBG_PRINT(ERR, "Failed to open file '%s'!\n", programFileName.c_str());
        return false;
    }
    filesystem::path fin_p = programFileName;
    string file_name = fin_p.stem().string();
    string block_name = blockNameFromFilename(file_name);


    mTapFile.blocks.clear();
    mTapFile.complete = true;
    mTapFile.validFileName = file_name;
    mTapFile.isAbcProgram = true;


    string line;
    int line_no;
    fin.seekg(0);
    Bytes data;
    string code;

    // Store program as a byte vector
    while (getline(fin, line)) {
        istringstream sin(line);
        sin >> line_no;
        /*
        while (sin.peek() == ' ') // skip spaces
            sin.get();  
         */
        getline(sin, code);

        data.push_back(0xd);
        data.push_back(line_no / 256);
        data.push_back(line_no % 256);
        /*
        if (code[0] != 0x0 && !(code[0] >= 'a' && code[0] <= 'z') && !(code[0] == '[' || code[0] == ']'))
            data.push_back(0x20); // add space after line no if line doesn't start with a label, '[' or ']'
         */
        for (int i = 0; i < 256 && code[i] != 0x0; data.push_back(code[i++]));
    }
    data.push_back(0x0d);
    data.push_back(0xff);
    fin.close();

    BytesIter data_iterator = data.begin();
    DBG_PRINT(DBG, "ABC Program byte vector:\n");
    logData(0x2900, data_iterator, data.size());


    // Create ATM block
    ATMBlock block;
    int block_no = 0;
    bool new_block = true;
    int count = 0;
    int load_address = 0x2900;
    BytesIter data_iter = data.begin();
    int block_sz;
    while (data_iter < data.end()) {

        if (new_block) {
            count = 0;
            if (data.end() - data_iter < 256)
                block_sz = data.end() - data_iter;
            else
                block_sz = 256;
            block.data.clear();
            block.tapeStartTime = -1;
            block.tapeEndTime = -1;
            block.hdr.execAdrHigh = 0xc2;
            block.hdr.execAdrLow = 0xb2;
            block.hdr.loadAdrHigh = load_address / 256;
            block.hdr.loadAdrLow = load_address % 256;
            for (int i = 0; i < 13; i++) {
                if (i < block_name.length())
                    block.hdr.name[i] = block_name[i];
                else
					block.hdr.name[i] = 0;
            }
            new_block = false;
        }

        
        if (count < block_sz) {
            block.data.push_back(*data_iter++);
            count++;
        } 
        else {
                block.hdr.lenHigh = 1;
                block.hdr.lenLow = 0;
                mTapFile.blocks.push_back(block);
                new_block = true;
                load_address += count;
                BytesIter block_iterator = block.data.begin();
                DBG_PRINT(DBG, "ABC Program block #%d:\n", block_no++);
                logData(load_address, block_iterator, block.data.size());
        }



        
    }
 
    block.hdr.lenHigh = count / 256;
    block.hdr.lenLow = count % 256;
    mTapFile.blocks.push_back(block);
    BytesIter block_iterator = block.data.begin();
    DBG_PRINT(DBG, "ABC Program block #%d:\n", block_no++);
    logData(load_address, block_iterator, block.data.size());



    return true;

}

bool AtomBasicCodec::getTAPFile(TAPFile& tapFile)
{
    tapFile = mTapFile;

    return true;

    return true;
}

bool AtomBasicCodec::setTAPFile(TAPFile& tapFile)
{
    mTapFile = tapFile;

    return true;
}
