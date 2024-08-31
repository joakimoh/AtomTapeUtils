#include "AtomBasicCodec.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Utility.h"
#include "Debug.h"
#include "BlockTypes.h"
#include <cstdlib>

using namespace std;


AtomBasicCodec::AtomBasicCodec(bool verbose, TargetMachine targetMachine) :mVerbose(verbose), mTargetMachine(targetMachine)
{
    // Intialise  dictionary of BBC Micro BASIC program tokens
    //
    // There is one for lookup of keyword from id (mTokenDictId)
    // and another for lookup of id from keyword (including abbreviations of keyword) (mTokenDictStr)
    for (int i = 0; i < mBBMTokens.size(); i++) {
        TokenEntry e = mBBMTokens[i];
        if (e.id1 != -1) {
            mTokenDictId[e.id1] = e;
            mTokenDictStr[e.fullT] = e;
            int min_len = e.shortT.length() - 1;
            for (int l = min_len; l < e.fullT.length(); l++) {
                string a = e.fullT.substr(0, l) + ".";
                mTokenDictStr[a] = e;
            }
        }
        if (e.id2 != -1) {
            mTokenDictId[e.id2] = e;
        }

        
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

    if (mTargetMachine <= BBC_MASTER)
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
    uint32_t exec_adr = Utility::bytes2uint(&file_block_iter->bbmHdr.execAdr[0], 4, true);
    uint32_t load_adr = Utility::bytes2uint(&file_block_iter->bbmHdr.loadAdr[0], 4, true);
    int data_len = Utility::bytes2uint(&file_block_iter->bbmHdr.blockLen[0], 2, true);;
    string name = file_block_iter->bbmHdr.name;

    Byte b;
    int read_bytes = 0;

    int tape_file_sz = tapeDataSz(tapeFile);

    if (mVerbose)
        cout << "\nTrying to encode program '" << tapeFile.blocks.front().bbmHdr.name << " ' as a BBC file...\n\n";

    int line_pos = -1;
    int line_no_low = 0;
    int line_no_high = 0;
    bool first_line = true;
    bool end_of_program = false;
    int n_blocks = 0;
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
            if (mTokenDictId.find(b) != mTokenDictId.end())
                fout << mTokenDictId[b].fullT;
            else
                fout << (char) b;
        }


    }

    if (mVerbose && (!end_of_program || unexpected_end_of_program))
        printf("Program '%s' didn't terminate with 0xff!\n", tapeFile.blocks.front().bbmHdr.name);

    if (mVerbose) {
        Utility::logTAPFileHdr(tapeFile);
        cout << "\nDone encoding program '" << tapeFile.blocks.front().bbmHdr.name << " ' as a BBC file...\n\n";
    }

    fout.close();

    return true;
}
bool AtomBasicCodec::encodeAtom(TapeFile& tapeFile, string& filePath, ofstream& fout)
{
    FileBlockIter file_block_iter = tapeFile.blocks.begin();
    BytesIter bi = file_block_iter->data.begin();
    int exec_adr = file_block_iter->atomHdr.execAdrHigh * 256 + file_block_iter->atomHdr.execAdrLow;
    int load_adr = file_block_iter->atomHdr.loadAdrHigh * 256 + file_block_iter->atomHdr.loadAdrLow;
    int data_len = file_block_iter->atomHdr.lenHigh * 256 + file_block_iter->atomHdr.lenLow;
    string name = file_block_iter->atomHdr.name;

    Byte b;
    int read_bytes = 0;

    int tape_file_sz = tapeDataSz(tapeFile);

    if (mVerbose && !(tapeFile.isBasicProgram)) {
        printf("File '%s' likely doesn\'t contain an Acorn Atom Basic Program => ABC file generated could be unreadable...\n", file_block_iter->atomHdr.name);
    }
    
    if (mVerbose)
        cout << "\nEncoding program '" << tapeFile.blocks.front().atomHdr.name << " ' as an ABC file...\n\n";

    int line_pos = -1;
    int line_no_high = 0;
    bool first_line = true;
    bool end_of_program = false;
    int n_blocks = 0;
    while (getTapeByte(tapeFile, file_block_iter, bi, read_bytes, b) && !end_of_program) {

        //
        // Format of Acorn Atom BASIC program in memory:
        // {<cr> <linehi> <linelo> <text>} <cr> <ff>
        //
        
            if (line_pos == 0) {

                if (b == 0xff) {
                    end_of_program = true;
                    int n_non_ABC_bytes = tape_file_sz - read_bytes;
                    if (mVerbose && n_non_ABC_bytes > 0) {
                        printf("Program file '%s' contains %d extra bytes after end of program - skipping this data for ABC file generation!\n", name.c_str(), n_non_ABC_bytes);
                    }
                }
                else {
                    line_no_high = int(b);
                    line_pos++;
                }
            }
            else if (line_pos == 1) {
                int line_no_low = int(b);
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
                fout << (char) b;
            }


            

    }

    if (mVerbose && !end_of_program)
        printf("Program '%s' didn't terminate with 0xff!\n", tapeFile.blocks.front().atomHdr.name);

    if (mVerbose) {
        Utility::logTAPFileHdr(tapeFile);
        cout << "\nDone encoding program '" << tapeFile.blocks.front().atomHdr.name << " ' as an ABC file...\n\n";
    }

    fout.close();

    return true;
}

bool AtomBasicCodec::decodeAtom(Bytes &data, TapeFile& tapeFile, string file_name, string block_name)
{

    BytesIter data_iterator = data.begin();
    if (DEBUG_LEVEL == DBG)
        Utility::logData(0x2900, data_iterator, data.size());


    // Create ATM block
    FileBlock block(ACORN_ATOM);
    bool new_block = true;
    int count = 0;
    int load_address = 0x2900;
    BytesIter data_iter = data.begin();
    int block_sz;
    int exec_adr = 0xc2b2;
    int tape_file_sz = 0;
    int n_blocks = 0;
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
            int data_len = block.data.size();
            if (mVerbose)
                Utility::logTAPBlockHdr(block, load_address, n_blocks);
            block.atomHdr.lenHigh = 1;
            block.atomHdr.lenLow = 0;
            tapeFile.blocks.push_back(block);
            new_block = true;
            load_address += count;
            BytesIter block_iterator = block.data.begin();
            tape_file_sz += data_len;
            n_blocks++;
            if (DEBUG_LEVEL == DBG)
                Utility::logData(load_address, block_iterator, data_len);
        }




    }

    // Catch last block
    {
        int data_len = block.data.size();
        if (mVerbose)
            Utility::logTAPBlockHdr(block, load_address, n_blocks);
        block.atomHdr.lenHigh = count / 256;
        block.atomHdr.lenLow = count % 256;
        tapeFile.blocks.push_back(block);
        BytesIter block_iterator = block.data.begin();

        tape_file_sz += data_len;
        n_blocks++;
        if (DEBUG_LEVEL == DBG)
            Utility::logData(load_address, block_iterator, data_len);
    }

    if (mVerbose) {
        int exec_adr = tapeFile.blocks[0].atomHdr.execAdrHigh * 256 + tapeFile.blocks[0].atomHdr.execAdrLow;
        int load_adr = tapeFile.blocks[0].atomHdr.loadAdrHigh * 256 + tapeFile.blocks[0].atomHdr.loadAdrLow;
        string tape_filename = tapeFile.blocks[0].atomHdr.name;
        if (mVerbose) 
            Utility::logTAPFileHdr(tapeFile);
        cout << "\nDone decoding ABC file '" << file_name << "'...\n\n'";
    }

    return true;
}

bool AtomBasicCodec::decodeBBM(Bytes &data, TapeFile& tapeFile, string file_name, string block_name)
{
    
    if (DEBUG_LEVEL == DBG) {
        BytesIter data_iterator = data.begin();
        Utility::logData(0xffff0e00, data_iterator, data.size());
    }

    // Create BBM block
    FileBlock block(BBC_MODEL_B);
    bool new_block = true;
    uint32_t count = 0;
    uint32_t file_load_address = 0xffff0e00;
    uint32_t load_address = file_load_address;
    BytesIter data_iter = data.begin();
    uint32_t block_sz;
    uint32_t exec_adr = file_load_address;
    uint32_t tape_file_sz = 0;
    uint32_t n_blocks = 0;

    while (data_iter < data.end()) {

        if (new_block) {
            count = 0;
            if (data.end() - data_iter < 256)
                block_sz = data.end() - data_iter;
            else
                block_sz = 256;

            if (!Utility::initTAPBlock(BBC_MODEL_B, block))
                return false;
            if (!Utility::encodeTAPHdr(block, block_name, file_load_address, load_address, exec_adr, n_blocks, block_sz))
                return false;

            new_block = false;

        }

        if (count < block_sz) {
            block.data.push_back(*data_iter++);
            count++;
        }
        if (count == block_sz) {

            if (data_iter == data.end())
                block.bbmHdr.blockFlag = 0x80;

            if (mVerbose)
                Utility::logTAPBlockHdr(block, tape_file_sz);

            tapeFile.blocks.push_back(block);
            new_block = true;
            load_address += block_sz;
            tape_file_sz += block_sz;
            n_blocks++;

            if (DEBUG_LEVEL == DBG) {
                BytesIter block_iterator = block.data.begin();
                Utility::logData(load_address, block_iterator, block_sz);
            }
        }

    }


    if (mVerbose) {
        Utility::logTAPFileHdr(tapeFile);
        cout << "\nDone decoding BBC Micro Tape File '" << file_name << "'...\n\n";
    }

    return true;
}

bool AtomBasicCodec::decode(string &fullPathFileName, TapeFile& tapeFile)
{

    ifstream fin(fullPathFileName);

    if (!fin) {
        printf("Failed to open file '%s'!\n", fullPathFileName.c_str());
        return false;
    }
    filesystem::path fin_p = fullPathFileName;
    string file_name = fin_p.stem().string();
    string block_name;

    block_name = Utility::blockNameFromFilename(mTargetMachine, file_name);

    if (mVerbose)
        cout << "\nDecoding ABC/BBC Micro file '" << fullPathFileName << "'...\n\n";

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

        if (sin.eof())
            code = "";
        else
            getline(sin, code);

        //cout << " LINE '" << line << "(" << code << ")" << "'\n";

        data.push_back(0xd);
        data.push_back(line_no / 256);
        data.push_back(line_no % 256);

        if (mTargetMachine <= BBC_MASTER) {
            string tCode;
            if (!tokenizeLine(code, tCode)) {
                cout << "Failed to tokenize line '" << code << "' ('" << tCode << "')\n";
                return false;
            }
            data.push_back((Byte) (4+tCode.length())); // line length + "size for <CR>,line no & line no"=4
            for (int i = 0; i < tCode.length(); data.push_back((Byte) tCode[i++]));

        }
        else {
            for (int i = 0; i < code.length(); data.push_back((Byte) code[i++]));
        }
    }
    data.push_back(0x0d);
    data.push_back(0xff);
    fin.close();

    if (mTargetMachine <= BBC_MASTER)
        return decodeBBM(data, tapeFile, file_name, block_name);
    else
        return decodeAtom(data, tapeFile, file_name, block_name);

}

bool AtomBasicCodec::decode(Bytes& data, string& fullPathFileName)
{
    TapeFile tape_file(ACORN_ATOM);
    if (mTargetMachine <= BBC_MASTER)
        tape_file.fileType = BBC_MODEL_B;

    ofstream fout(fullPathFileName);
    if (!fout) {
        printf("Can't write to program file '%s'!\n", fullPathFileName.c_str());
        return false;
    }

    filesystem::path fin_p = fullPathFileName;
    string file_name = fin_p.stem().string();
    string block_name;
    block_name = Utility::blockNameFromFilename(mTargetMachine, file_name);

    bool success;
    if (mTargetMachine <= BBC_MASTER)
        success = decodeBBM(data, tape_file, file_name, block_name);
    else
        success = decodeAtom(data, tape_file, file_name, block_name);

    return encode(tape_file, fullPathFileName);

}

bool AtomBasicCodec::tokenizeLine(string &line, string& tCode)
{
    string space, token;
    TokenEntry entry;
    int pos = 0;
    bool start_of_statement = true;
    bool within_string = false;
    bool fun_or_proc = false;
    string code = line;

    tCode = "";
    while (code.length() > 0) {
        if (!getKeyWord(fun_or_proc, start_of_statement, within_string, code, space, token, entry)) {
            return false;
        }
        if (entry.fullT != "") {
            if (entry.id2 != -1 && start_of_statement) {
                tCode += space + (char) (entry.id2); // start of a statement <=> potential left side of assigment => id2 applicable
            } 
            else {
                tCode += space + (char) entry.id1;
            }
            
        }
        else {
            if (token == ":")
                start_of_statement = true;
            else
                start_of_statement = false;
            if (token == "\"")
                within_string = !within_string;

            tCode += space + token;
        }

    }

    return true;
}

bool AtomBasicCodec::isDelimiter(char c)
{
    const string delimiters = "'+-*/^!,<>?;[]:*{}@\"#$%&'()=~\|_£";

    for (int i = 0; i < delimiters.length(); i++) {
        if (c == delimiters[i])
            return true;
    }
    return false;
}

// Advance one token
bool AtomBasicCodec::nextToken(string& text, string& token)
{
    // Check for end of line
    if (text.length() == 0) {
        token = "";
        return true;
    }
    // 
    // Check for number
    int pos = 0;
    while (pos < text.length() && !isDelimiter(text[pos]) && text[pos] >= '0' && text[pos] <= '9')
        pos++;
    if (pos > 0) {
        token = text.substr(0, pos);
        text = text.substr(pos);
       
        return true;
    }

    // Check for non-number
    while (pos < text.length() && !isDelimiter(text[pos]) && text[pos] != ' ')
        pos++;
    if (pos > 0) {
        token = text.substr(0, pos);
        text = text.substr(pos);

        return true;
    }

    { // Most be a delimiter then
        if (pos < text.length() && isDelimiter(text[pos])) {
            pos++;
        }
        if (pos > 0) {
            token = text.substr(0, pos);
            text = text.substr(pos);

            return true;
        }
    }

    return false;
}

bool AtomBasicCodec::getKeyWord(bool &fun_or_proc, bool startOfStatement, bool withinString, string& text, string &space, string& token, TokenEntry& entry)
{
    int pos = 0;

    token = "";
    const TokenEntry empty = { "", "", {-1,-1} };
    space = "";

    entry = empty;

    

    // Skip initial space
    while (pos < text.length() && text[pos] == ' ')
        pos++;
    if (pos > 0) {
        space = text.substr(0, pos);
        text = text.substr(pos);
        pos = 0;
    }

    if (withinString)
        return nextToken(text, token);

    if (fun_or_proc) {
        fun_or_proc = false;
        return nextToken(text, token);     
    }


    // Get start of keyword that consists of pure upper-case letters
    int first_matched_pos = -1;
    TokenEntry first_matched_keyword = empty;
    while (pos < text.length() && text[pos] >= 'A' && text[pos] <= 'Z') {
        token = text.substr(0, pos+1);
        if (pos > 0 && mTokenDictStr.find(token) != mTokenDictStr.end()) {
            // Some full keywords could be a sub string of another one:
            //  END(PROC), ERR(OR), GET($), INKEY($), MOD(E)
            // If a match for the shorter keyword is made, we need to
            // check that the longer keyword is not matched as well
            // If the longer is matched, that one shall be used and not
            // the shorter one.
            if (first_matched_pos == -1) {
                first_matched_pos = pos;
                first_matched_keyword = mTokenDictStr[token];
            }
            else {
                text = text.substr(pos+1);
                entry = mTokenDictStr[token];
                return true;
            }
        }
        pos++;

     }

    // If a complete keyword was matched previously, then use it!
    if (first_matched_pos != -1) {
        (void) matchAgainstKeyword(text, first_matched_pos + 1, token, entry);
        if (entry.fullT == "FN" || entry.fullT == "PROC")
            fun_or_proc = true;
        return true;
    }

    if (pos == 0) { // Cannot be a keyword as not starting with upper-case letters
        return nextToken(text, token);
    }

    // Get first letter of potentially remaining part of keyword that consists of either '$', '(' or '$('
    if (pos < text.length() && (text[pos] == '$' || text[pos] == '(')) pos++;

    // Check for match [A-Z]+('$'|'(')
    if (matchAgainstKeyword(text, pos, token, entry))
        return true;

    // Get last part of keyword that potentially could be '(' (if keyword ends with '$(')
    if (pos < text.length() && text[pos] == '(') pos++;

    // Check for match [A-Z]+'$('
    if (matchAgainstKeyword(text, pos, token, entry))
        return true;

    // Get potentially ending '.'
    if (pos < text.length() && text[pos] == '.') pos++;

    // Check the completed token against keyword
    if (matchAgainstKeyword(text, pos, token, entry))
        return true;
    else
        return nextToken(text, token);


    return true;

}

bool AtomBasicCodec::matchAgainstKeyword(string& text, int pos, string& token, TokenEntry& entry) {
    token = text.substr(0, pos);
    if (mTokenDictStr.find(token) != mTokenDictStr.end()) {
        text = text.substr(pos);
        entry = mTokenDictStr[token];
        return true;
    }
    return false;
}