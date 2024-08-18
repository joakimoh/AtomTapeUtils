#include "AtomBasicCodec.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Utility.h"
#include "Debug.h"
#include "BlockTypes.h"

using namespace std;


AtomBasicCodec::AtomBasicCodec(bool verbose, bool bbcMicro) :mVerbose(verbose), mBbcMicro(bbcMicro)
{
    // Intialise  dictionary of BBC Micro BASIC program tokens
    for (int i = 0; i < mBBMTokens.size(); i++) {
        TokenEntry e = mBBMTokens[i];
        if (e.id1 != -1)
            mTokenDict[e.id1] = e;
        if (e.id2 != -1)
            mTokenDict[e.id2] = e;
    }
}

bool AtomBasicCodec::encode(TapeFile& tapeFile, string& filePath)
{
    if (mVerbose && tapeFile.blocks.empty()) {
        printf("TAP File '%s' is empty => no Program file created!\n", filePath.c_str());
        return false;
    }

    ofstream fout(filePath);
    if (!fout) {
        printf("Can't write to program file '%s'!\n", filePath.c_str());
        return false;
    }

    if (mBbcMicro)
        return encodeBBM(tapeFile, filePath, fout);
    else
        return encodeAtom(tapeFile, filePath, fout);
}

bool getTapeByte(TapeFile& f, FileBlockIter& fi, BytesIter& bi, int &read_bytes, Byte &tapeByte)
{
    if (fi >= f.blocks.end())
        return false;
    while (fi < f.blocks.end() && bi >= fi->data.end()) {
        fi++;
        if (fi < f.blocks.end())
            bi = fi -> data.begin();
    }
    if (fi >= f.blocks.end() || bi >= fi->data.end())
        return false;

    tapeByte = *bi;

    bi++;

    read_bytes++;

    return true;
}

int tapeDataSz(TapeFile& f)
{
    int sz = 0;
    for (int i = 0; i < f.blocks.size(); i++)
        sz += f.blocks[i].data.size();
    return sz;
}


bool AtomBasicCodec::encodeBBM(TapeFile& tapeFile, string& filePath, ofstream& fout)
{
    FileBlockIter file_block_iter = tapeFile.blocks.begin();
    BytesIter bi = file_block_iter->data.begin();
    int exec_adr = bytes2uint(&file_block_iter->bbmHdr.execAdr[0], 4, true);
    int load_adr = bytes2uint(&file_block_iter->bbmHdr.loadAdr[0], 4, true);
    int data_len = bytes2uint(&file_block_iter->bbmHdr.blockLen[0], 2, true);;
    string name = file_block_iter->bbmHdr.name;

    Byte b;
    int read_bytes = 0;

    int tape_sz = tapeDataSz(tapeFile);

    if (mVerbose)
        cout << "\nTrying to encode program '" << tapeFile.blocks.front().bbmHdr.name << " ' as a BBC file...\n\n";

    int line_pos = -1;
    int line_no_low = 0;
    int line_no_high = 0;
    bool first_line = true;
    bool end_of_program = false;
    unsigned tape_file_sz = 0;
    unsigned n_blocks = 0;
    bool unexpected_end_of_program = false;
    int line_len;
    
    while (getTapeByte(tapeFile, file_block_iter, bi, read_bytes, b) && !end_of_program) {
        
        if (line_pos == 0) {
            if (b == 0xff) {
                end_of_program = true;
                int n_non_ABC_bytes = tape_file_sz - read_bytes;
                if (mVerbose && n_non_ABC_bytes > 0) {
                    unexpected_end_of_program = true;
                    printf("Program file '%s' contains %d extra bytes after end of program - skipping this data for BBC file generation!\n", name.c_str(), n_non_ABC_bytes);
                }
            }
            else {
                line_no_high = b;
                line_pos++;
            }
        }
        else if (line_pos == 1) {
            line_no_low = b;
            line_pos++;
        }
        else if (line_pos == 2) {
            line_len = b;
            line_pos = -1;
            int line_no = line_no_high * 256 + line_no_low;
            char line_no_s[7];
            sprintf(line_no_s, "%5d", line_no);
            fout << line_no_s;
        }
        else if (b == 0xd) {
            line_pos = 0;
            if (!first_line) {
                fout << "\n";
            }
            first_line = false;

        }
        else {
            if (mTokenDict.find(b) != mTokenDict.end())
                fout << mTokenDict[b].fullT;
            else
                fout << (char) b;
        }


    }

    if (mVerbose && (!end_of_program || unexpected_end_of_program))
        printf("Program '%s' didn't terminate with 0xff!\n", tapeFile.blocks.front().bbmHdr.name);

    if (mVerbose) {
        unsigned exec_adr = bytes2uint(&tapeFile.blocks[0].bbmHdr.execAdr[0], 4, true);
        unsigned load_adr = bytes2uint(&tapeFile.blocks[0].bbmHdr.loadAdr[0], 4, true);
        string tape_filename = tapeFile.blocks[0].bbmHdr.name;
        if (mVerbose) {
            printf("\n%13s %4x %4x %4x %2d %5d\n", tape_filename.c_str(),
                load_adr, load_adr + tape_file_sz - 1, exec_adr, n_blocks, tape_file_sz
            );
        }
        cout << "\nDone encoding program '" << tapeFile.blocks.front().bbmHdr.name << " ' as a BBC file...\n\n";
    }

    fout.close();

    return true;
}
bool AtomBasicCodec::encodeAtom(TapeFile& tapeFile, string& filePath, ofstream& fout)
{
    FileBlockIter file_block_iter = tapeFile.blocks.begin();

    if (mVerbose && !(tapeFile.isBasicProgram)) {
        printf("File '%s' likely doesn\'t contain an Acorn Atom Basic Program => ABC file generated could be unreadable...\n", file_block_iter->atomHdr.name);
    }
    
    if (mVerbose)
        cout << "\nEncoding program '" << tapeFile.blocks.front().atomHdr.name << " ' as an ABC file...\n\n";

    int line_pos = -1;
    int line_no_high = 0;
    bool first_line = true;
    bool end_of_program = false;
    unsigned tape_file_sz = 0;
    unsigned n_blocks = 0;
    while (file_block_iter < tapeFile.blocks.end()) {

        //
        // Format of Acorn Atom BASIC program in memory:
        // {<cr> <linehi> <linelo> <text>} <cr> <ff>
        //
        BytesIter bi = file_block_iter->data.begin();
        int exec_adr = file_block_iter->atomHdr.execAdrHigh * 256 + file_block_iter->atomHdr.execAdrLow;
        int load_adr = file_block_iter->atomHdr.loadAdrHigh * 256 + file_block_iter->atomHdr.loadAdrLow;
        int data_len = file_block_iter->atomHdr.lenHigh * 256 + file_block_iter->atomHdr.lenLow;
        string name = file_block_iter->atomHdr.name;
        tape_file_sz += data_len;

        while (bi < file_block_iter->data.end() && !end_of_program) {
            if (line_pos == 0) {
                if (*bi == 0xff) {
                    bi++;
                    end_of_program = true;
                    int n_non_ABC_bytes = file_block_iter->data.end() - bi;
                    if (mVerbose && file_block_iter->data.end() - bi > 0) {
                        printf("Program file '%s' contains %d extra bytes after end of program - skipping this data for ABC file generation!\n", name.c_str(), n_non_ABC_bytes);
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
                sprintf(line_no_s, "%5d", line_no);
                fout << line_no_s;
            }
            else if (*bi == 0xd) {
                line_pos = 0;
                bi++;
                if (!first_line) {
                    fout << "\n";
                }
                first_line = false;

            }
            else {
                fout << (char)*bi++;
            }

        }

        if (mVerbose)
            printf("%13s %.4x %.4x %.3d\n", name.c_str(), load_adr, exec_adr, data_len);

        file_block_iter++;
        n_blocks++;
    }

    if (mVerbose && !end_of_program)
        printf("Program '%s' didn't terminate with 0xff!\n", tapeFile.blocks.front().atomHdr.name);

    if (mVerbose) {
        unsigned exec_adr = tapeFile.blocks[0].atomHdr.execAdrHigh * 256 + tapeFile.blocks[0].atomHdr.execAdrLow;
        unsigned load_adr = tapeFile.blocks[0].atomHdr.loadAdrHigh * 256 + tapeFile.blocks[0].atomHdr.loadAdrLow;
        string tape_filename = tapeFile.blocks[0].atomHdr.name;
        if (mVerbose) {
            printf("\n%13s %4x %4x %4x %2d %5d\n", tape_filename.c_str(),
                load_adr, load_adr + tape_file_sz - 1, exec_adr, n_blocks, tape_file_sz
            );
        }
        cout << "\nDone encoding program '" << tapeFile.blocks.front().atomHdr.name << " ' as an ABC file...\n\n";
    }

    fout.close();

    return true;
}


bool AtomBasicCodec::decode(string &programFileName, TapeFile& tapeFile)
{

    ifstream fin(programFileName);

    if (!fin) {
        printf("Failed to open file '%s'!\n", programFileName.c_str());
        return false;
    }
    filesystem::path fin_p = programFileName;
    string file_name = fin_p.stem().string();
    string block_name = blockNameFromFilename(file_name);

    if (mVerbose)
        cout << "\nDecoding ABC file '" << programFileName << "'...\n\n";


    tapeFile.blocks.clear();
    tapeFile.complete = true;
    tapeFile.validFileName = file_name;
    tapeFile.isBasicProgram = true;


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
    if (DEBUG_LEVEL == DBG)
        logData(0x2900, data_iterator, data.size());


    // Create ATM block
    FileBlock block(AtomBlock);
    bool new_block = true;
    int count = 0;
    int load_address = 0x2900;
    BytesIter data_iter = data.begin();
    int block_sz;
    unsigned exec_adr = 0xc2b2;
    unsigned tape_file_sz = 0;
    unsigned n_blocks = 0;
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
            block.atomHdr.execAdrHigh = (exec_adr >> 8) & 0xff;
            block.atomHdr.execAdrLow = exec_adr & 0xff;
            block.atomHdr.loadAdrHigh = load_address / 256;
            block.atomHdr.loadAdrLow = load_address % 256;
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
            unsigned data_len = block.data.size();
            if (mVerbose)
                printf("%13s %.4x %.4x %.4x %3d %5d\n", block_name.c_str(), load_address, load_address + data_len - 1, exec_adr, n_blocks, data_len);
            block.atomHdr.lenHigh = 1;
            block.atomHdr.lenLow = 0;
            tapeFile.blocks.push_back(block);
            new_block = true;
            load_address += count;
            BytesIter block_iterator = block.data.begin();
            tape_file_sz += data_len;
            n_blocks++;
            if (DEBUG_LEVEL == DBG)
                logData(load_address, block_iterator, data_len);
        }



        
    }
 
    // Catch last block
    {
        unsigned data_len = block.data.size();
        if (mVerbose)
            printf("%13s %.4x %.4x %.4x %3d %5d\n", block_name.c_str(), load_address, load_address + data_len - 1, exec_adr, n_blocks, data_len);
        block.atomHdr.lenHigh = count / 256;
        block.atomHdr.lenLow = count % 256;
        tapeFile.blocks.push_back(block);
        BytesIter block_iterator = block.data.begin();
        
        tape_file_sz += data_len;
        n_blocks++;
        if (DEBUG_LEVEL == DBG)
            logData(load_address, block_iterator, data_len);
    }

    if (mVerbose) {
        unsigned exec_adr = tapeFile.blocks[0].atomHdr.execAdrHigh * 256 + tapeFile.blocks[0].atomHdr.execAdrLow;
        unsigned load_adr = tapeFile.blocks[0].atomHdr.loadAdrHigh * 256 + tapeFile.blocks[0].atomHdr.loadAdrLow;
        string tape_filename = tapeFile.blocks[0].atomHdr.name;
        if (mVerbose) {
            printf("\n%13s %4x %4x %4x %3d %5d\n", tape_filename.c_str(),
                load_adr, load_adr + tape_file_sz - 1, exec_adr, n_blocks, tape_file_sz
            );
        }
        cout << "\nDone decoding ABC file '" << programFileName << "'...\n\n'";
    }


    return true;

}
