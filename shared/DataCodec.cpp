#include "DataCodec.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include "TAPCodec.h"
#include "UEFCodec.h"
#include "Utility.h"
#include "Debug.h"

using namespace std;

namespace fs = std::filesystem;

/*
 * Create DATA Codec and initialise it with a TAP
 * file structure. If the file is not complete,
 * then 'complete' shall be set to false.
 */
DataCodec::DataCodec(TAPFile& tapFile): mTapFile(tapFile)
{

}

DataCodec::DataCodec()
{

}

/*
 * Encode TAP File structure as DATA file
 */
bool DataCodec::encode(string& filePath)
{

    if (mTapFile.blocks.empty()) {
        cout << "Empty TAP file => no DATA to encode!\n";
        return false;
    }

    ofstream fout(filePath);
    if (!fout) {
        DBG_PRINT(ERR, "Can't write to DATA file '%s'!\n", filePath.c_str());
        return false;
    }

    ATMBlockIter ATM_block_iter = mTapFile.blocks.begin();

    int c = 0;
    bool new_line = true;

    while (ATM_block_iter < mTapFile.blocks.end()) {

        BytesIter bi = ATM_block_iter->data.begin();
        int exec_adr = ATM_block_iter->hdr.execAdrHigh * 256 + ATM_block_iter->hdr.execAdrLow;
        int load_adr = ATM_block_iter->hdr.loadAdrHigh * 256 + ATM_block_iter->hdr.loadAdrLow;
        int data_len = ATM_block_iter->hdr.lenHigh * 256 + ATM_block_iter->hdr.lenLow;
        string name = ATM_block_iter->hdr.name;
        char s[64];



        BytesIter li;
        int line_sz = 16;
        while (bi < ATM_block_iter->data.end()) {

            if (new_line) {
                if (ATM_block_iter->data.end() - bi < 16)
                    line_sz = ATM_block_iter->data.end() - bi;
                li = bi;
                sprintf_s(s, "%.4x ", load_adr);
                fout << s;
                c = 0;
                new_line = false;
            }

            sprintf_s(s, "%.2x ", int(*bi++));
            fout << s;

            if (c == line_sz - 1) {
                new_line = true;

                load_adr += 16;
                c = 0;
                fout << " ";
                for (int j = 0; j < 16 - line_sz; j++)
                    fout << "   ";
                for (int i = 0; i < line_sz; i++) {
                    Byte b = *li++;
                    if (b >= 0x20 && b <= 0x7e)
                        fout << (char)b;
                    else
                        fout << ".";
                }
                fout << "\n";
            }

            c++;


        }
        ATM_block_iter++;
    }

    fout.close();

    DBG_PRINT(DBG, "DATA file '%s' created from %d blocks...\n", filePath.c_str(), mTapFile.blocks.size());


    return true;
}

bool DataCodec::decode2Bytes(string& dataFileName, int &startAdress, Bytes &data)
{
    ifstream fin(dataFileName);

    if (!fin) {
        cout << "Failed to open file '" << dataFileName << "'\n";
        return false;
    }
    filesystem::path fin_p = dataFileName;

    string line;
    int address;

    fin.seekg(0);
    string values;

    // Store data as a byte vector
    bool first_line = true;
    while (getline(fin, line)) {

        istringstream sin(line);
        sin >> hex >> address;

        getline(sin, values);

        if (first_line) {
            startAdress = address;
            first_line = false;
        }
        // Determine the #values on the row (16 on all but the last one possibly)
        istringstream v_stream(values);
        char c;
        int n_space = 0;
        char pc = ' ';
        int n_values = 0;
        while ((v_stream >> std::noskipws >> c) && n_space < 2) {
            if ((char)c == (char)' ')
                n_space++;
            if (c != ' ' && pc == ' ') {
                n_values++;
                n_space = 0;
            }
            pc = c;
        }
        int val;
        v_stream = istringstream(values);
        int n = 0;
        while (v_stream >> hex >> val && n++ < n_values)
            data.push_back(val);

    }
    fin.close();

    return true;
}

/*
 * Decode DATA file as TAP File structure
 */
bool DataCodec::decode(string& dataFileName)
{
    Bytes data;
    int load_address;

    if (!decode2Bytes(dataFileName, load_address, data)) {
        DBG_PRINT(DBG, "Failed to decode bytes");
        return false;
    }
 
    filesystem::path fin_p = dataFileName;
    string file_name = fin_p.stem().string();
    string block_name = blockNameFromFilename(file_name);

    mTapFile.blocks.clear();
    mTapFile.complete = true;
    mTapFile.validFileName = file_name;
    mTapFile.isAbcProgram = true;


    BytesIter data_iterator = data.begin();


    // Create ATM block
    ATMBlock block;
    int block_no = 0;
    bool new_block = true;
    int count = 0;
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
            // logData(load_address, block_iterator, block.data.size());
        }




    }

    block.hdr.lenHigh = count / 256;
    block.hdr.lenLow = count % 256;
    mTapFile.blocks.push_back(block);
    BytesIter block_iterator = block.data.begin();
    //logData(load_address, block_iterator, block.data.size());



    return true;
}

/*
 * Get the codec's TAP file
 */
bool DataCodec::getTAPFile(TAPFile& tapFile)
{
	tapFile = mTapFile;

	return true;
}

/*
 * Reinitialise codec with a new TAP file
 */
bool DataCodec::setTAPFile(TAPFile& tapFile)
{
	mTapFile = tapFile;

	return true;
}