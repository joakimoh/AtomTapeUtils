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
 * Create DATA Codec.
 */
DataCodec::DataCodec(bool verbose): mVerbose(verbose)
{

}

bool DataCodec::encode(TapeFile& tapeFile, string& filePath)
{
    if (tapeFile.blocks.empty()) {
        cout << "Empty TAP file => no DATA to encode!\n";
        return false;
    }

    ofstream fout(filePath);
    if (!fout) {
        printf("Can't write to DATA file '%s'!\n", filePath.c_str());
        return false;
    }

    if (tapeFile.fileType == FileType::AtomFile)
        return encodeAtom(tapeFile, filePath, fout);
    else
        return encodeBBM(tapeFile, filePath, fout);
}

/*
 * Encode BBC Micro TAP File structure as DATA file
 */
bool DataCodec::encodeBBM(TapeFile& tapeFile, string& filePath, ofstream& fout)
{

    if (mVerbose)
        cout << "\nEncoding BBC Micro program '" << tapeFile.blocks[0].bbmHdr.name << "' as a DATA file...\n\n";

    FileBlockIter file_block_iter = tapeFile.blocks.begin();

    int c = 0;
    bool new_line = true;
    unsigned tape_file_sz = 0;
    while (file_block_iter < tapeFile.blocks.end()) {

        BytesIter bi = file_block_iter->data.begin();
        int exec_adr = bytes2uint(&file_block_iter->bbmHdr.execAdr[0], 4, true);
        int file_load_adr = bytes2uint(&file_block_iter->bbmHdr.loadAdr[0], 4, true);
        int data_len = bytes2uint(&file_block_iter->bbmHdr.blockLen[0], 2, true);
        int block_no = bytes2uint(&file_block_iter->bbmHdr.blockNo[0], 2, true);
        int flag = file_block_iter->bbmHdr.blockFlag;
        int locked = file_block_iter->bbmHdr.locked;
        string name = file_block_iter->bbmHdr.name;
        char s[64];

        int load_adr = file_load_adr + tape_file_sz;
        tape_file_sz += data_len;

        if (mVerbose) {
            printf("%10s %.8x %.8x %.8x %.4x %.4x %.2x\n", name.c_str(),
                file_load_adr, load_adr + data_len - 1, exec_adr, block_no, data_len, flag
            );
        }

        
        BytesIter li;
        int line_sz = 16;
        while (bi < file_block_iter->data.end()) {

            if (new_line) {
                if (file_block_iter->data.end() - bi < 16)
                    line_sz = file_block_iter->data.end() - bi;
                li = bi;
                sprintf(s, "%.4x ", load_adr);
                fout << s;
                c = 0;
                new_line = false;
            }

            sprintf(s, "%.2x ", int(*bi++));
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
        file_block_iter++;
    }

    fout.close();

    if (mVerbose) {
        int exec_adr = bytes2uint(&tapeFile.blocks[0].bbmHdr.execAdr[0], 4, true);
        int file_load_adr = bytes2uint(&tapeFile.blocks[0].bbmHdr.loadAdr[0], 4, true);
        int data_len = bytes2uint(&tapeFile.blocks[0].bbmHdr.blockLen[0], 2, true);
        int block_no = bytes2uint(&tapeFile.blocks[0].bbmHdr.blockNo[0], 2, true);
        string name = tapeFile.blocks[0].bbmHdr.name;
        printf("\n%10s %.8x %.8x %.8x %.4x %.4x\n", name.c_str(),
            file_load_adr, file_load_adr + tape_file_sz - 1, exec_adr, block_no, tape_file_sz
        );
        cout << "\nDone encoding program '" << tapeFile.blocks[0].bbmHdr.name << "' as a DATA file...\n\n";
    }

    return true;
}


/*
 * Encode Acron Atom TAP File structure as DATA file
 */
bool DataCodec::encodeAtom(TapeFile &tapeFile, string& filePath, ofstream &fout)
{

    if (mVerbose)
        cout << "\nEncoding Acorn Atom program '" << tapeFile.blocks[0].atomHdr.name << "' as a DATA file...\n\n";

    FileBlockIter file_block_iter = tapeFile.blocks.begin();

    int c = 0;
    bool new_line = true;
    unsigned block_no = 0;
    unsigned tape_file_sz = 0;
    while (file_block_iter < tapeFile.blocks.end()) {

        BytesIter bi = file_block_iter->data.begin();
        int exec_adr = file_block_iter->atomHdr.execAdrHigh * 256 + file_block_iter->atomHdr.execAdrLow;
        int load_adr = file_block_iter->atomHdr.loadAdrHigh * 256 + file_block_iter->atomHdr.loadAdrLow;
        int data_len = file_block_iter->atomHdr.lenHigh * 256 + file_block_iter->atomHdr.lenLow;
        string name = file_block_iter->atomHdr.name;
        char s[64];

        if (mVerbose) {
            printf("%13s %4x %4x %4x %2d %5d\n", name.c_str(),
                load_adr, load_adr + data_len - 1, exec_adr, block_no, data_len
            );
        }
        
        block_no++;
        tape_file_sz += data_len;


        BytesIter li;
        int line_sz = 16;
        while (bi < file_block_iter->data.end()) {

            if (new_line) {
                if (file_block_iter->data.end() - bi < 16)
                    line_sz = file_block_iter->data.end() - bi;
                li = bi;
                sprintf(s, "%.4x ", load_adr);
                fout << s;
                c = 0;
                new_line = false;
            }

            sprintf(s, "%.2x ", int(*bi++));
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
        file_block_iter++;
    }

    fout.close();

    if (mVerbose) {
        int exec_adr = tapeFile.blocks[0].atomHdr.execAdrHigh * 256 + tapeFile.blocks[0].atomHdr.execAdrLow;
        int load_adr = tapeFile.blocks[0].atomHdr.loadAdrHigh * 256 + tapeFile.blocks[0].atomHdr.loadAdrLow;
        int data_len = tapeFile.blocks[0].atomHdr.lenHigh * 256 + tapeFile.blocks[0].atomHdr.lenLow;
        string name = tapeFile.blocks[0].atomHdr.name;
        printf("\n%13s %4x %4x %4x %2d %5d\n", name.c_str(),
            load_adr, load_adr + tape_file_sz - 1, exec_adr, block_no, tape_file_sz
        );
        cout << "\nDone encoding program '" << tapeFile.blocks[0].atomHdr.name << "' as a DATA file...\n\n";
    }

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
bool DataCodec::decode(string& dataFileName, TapeFile &tapeFile)
{
    Bytes data;
    int load_address;

    if (!decode2Bytes(dataFileName, load_address, data)) {
        printf("Failed to decode bytes for file '%s'\n", dataFileName.c_str());
        return false;
    }

    if (mVerbose)
        cout << "\nDecoding DATA file '" << dataFileName << "'...\n\n";
 
    filesystem::path fin_p = dataFileName;
    string file_name = fin_p.stem().string();
    string block_name = blockNameFromFilename(file_name);

    tapeFile.blocks.clear();
    tapeFile.complete = true;
    tapeFile.validFileName = file_name;
    tapeFile.isBasicProgram = true;


    BytesIter data_iterator = data.begin();


    // Create ATM blocks
    FileBlock block(AtomBlock);
    int block_no = 0;
    bool new_block = true;
    int count = 0;
    BytesIter data_iter = data.begin();
    int block_sz;
    unsigned exec_adr = 0xc2b2;
    unsigned tape_file_sz = 0;
    while (data_iter < data.end()) {

        if (new_block) {
            count = 0;
            if (data.end() - data_iter < 256)
                block_sz = data.end() - data_iter;
            else
                block_sz = 256;
            tape_file_sz += block_sz;
            block.data.clear();
            block.tapeStartTime = -1;
            block.tapeEndTime = -1;
            block.atomHdr.execAdrHigh = (exec_adr >> 8) & 0xff;
            block.atomHdr.execAdrLow = exec_adr & 0xff;
            block.atomHdr.loadAdrHigh = (load_address >> 8) & 0xff;;
            block.atomHdr.loadAdrLow = load_address & 0xff;
            for (int i = 0; i < sizeof(block.atomHdr.name); i++) {
                if (i < block_name.length())
                    block.atomHdr.name[i] = block_name[i];
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
            if (mVerbose) {
                printf("%13s %4x %4x %4x %2d %5d\n", block_name.c_str(),
                    load_address, load_address + block_sz - 1, exec_adr, block_no, block_sz
                );
            }
            load_address += count;
            block_no++;
            BytesIter block_iterator = block.data.begin();
            // if (DEBUG_LEVEL == DBG) logData(load_address, block_iterator, block.data.size());
        }


    }

    // Catch last block
    block.atomHdr.lenHigh = count / 256;
    block.atomHdr.lenLow = count % 256;
    tapeFile.blocks.push_back(block);

    if (data.end() - data_iter < 256)
        block_sz = data.end() - data_iter;
    else
        block_sz = 256;
    tape_file_sz += block_sz;
    if (mVerbose) {
        printf("%13s %4x %4x %4x %2d %5d\n", block_name.c_str(),
            load_address, load_address + block_sz - 1, exec_adr, block_no, block_sz
        );
    }
    //if (DEBUG_LEVEL == DBG) logData(load_address, block_iterator, block.data.size());

    if (mVerbose) {
        unsigned exec_adr = tapeFile.blocks[0].atomHdr.execAdrHigh * 256 + tapeFile.blocks[0].atomHdr.execAdrLow;
        unsigned load_adr = tapeFile.blocks[0].atomHdr.loadAdrHigh * 256 + tapeFile.blocks[0].atomHdr.loadAdrLow;
        string atom_filename = tapeFile.blocks[0].atomHdr.name;
        if (mVerbose) {
            printf("\n%13s %4x %4x %4x %2d %5d\n", atom_filename.c_str(),
                load_adr, load_adr + tape_file_sz - 1, exec_adr, block_no, tape_file_sz
            );
        }
        cout << "\nDone decoding DATA file '" << dataFileName << "'...\n\n";
    }

    return true;
}
