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
    int tape_file_sz = 0;
    while (file_block_iter < tapeFile.blocks.end()) {

        BytesIter bi = file_block_iter->data.begin();
        uint32_t file_load_adr = bytes2uint(&file_block_iter->bbmHdr.loadAdr[0], 4, true);
        int block_sz = bytes2uint(&file_block_iter->bbmHdr.blockLen[0], 2, true);
        char s[64];

        int load_adr = file_load_adr + tape_file_sz;
        tape_file_sz += block_sz;

        if (mVerbose) {
            logTAPBlockHdr(*file_block_iter, tape_file_sz);
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
        logTAPFileHdr(tapeFile);
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
    int block_no = 0;
    int tape_file_sz = 0;
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
    uint32_t address;

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
bool DataCodec::decode(string& dataFileName, TapeFile &tapeFile, bool bbcMicro)
{
    Bytes data;
    int load_adr;

    if (!decode2Bytes(dataFileName, load_adr, data)) {
        printf("Failed to decode bytes for file '%s'\n", dataFileName.c_str());
        return false;
    }

    if (mVerbose)
        cout << "\nDecoding DATA file '" << dataFileName << "' with " << data.size() << " bytes of data...\n\n";
 
    filesystem::path fin_p = dataFileName;
    string file_name = fin_p.stem().string();

    string block_name;
    if (bbcMicro)
        block_name = bbmBlockNameFromFilename(file_name);
    else
        block_name = atomBlockNameFromFilename(file_name);

    tapeFile.blocks.clear();
    tapeFile.complete = true;
    tapeFile.validFileName = file_name;
    tapeFile.isBasicProgram = true;

    FileBlock block(AtomBlock);
    int exec_adr = 0xc2b2;
    int file_load_adr = 0x2900;
    if (bbcMicro) {
        block.blockType = FileBlockType::BBCMicroBlock;
        file_load_adr = 0xffff0e00;
        exec_adr = load_adr;
    }

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
                block_sz = data.end() - data_iter;
            else
                block_sz = 256;
            tape_file_sz += block_sz;

            
            if (!encodeTAPHdr(block, block_name, file_load_adr, load_adr, exec_adr, block_no, block_sz)) {
                cout << "Failed to encode TAP file header for file '" << dataFileName << "'!\n";
                return false;
            }
            new_block = false;

        }

        if (count < block_sz) {
            block.data.push_back(*data_iter++);
            count++;
        }
        if (count == block_sz) {

            if (mVerbose)
                logTAPBlockHdr(block, tape_file_sz);

            tapeFile.blocks.push_back(block);
            new_block = true;
            
            load_adr += count;
            block_no++;
            //BytesIter block_iterator = block.data.begin();
            // if (DEBUG_LEVEL == DBG) logData(load_address, block_iterator, block.data.size());
        }


    }


    //if (DEBUG_LEVEL == DBG) logData(load_address, block_iterator, block.data.size());

    if (mVerbose)
        logTAPFileHdr(tapeFile);

    if (mVerbose)
        cout << "\nDone decoding DATA file '" << dataFileName << "'...\n\n";
 
    return true;
}