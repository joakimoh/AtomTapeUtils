#include "AtomBasicCodec.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Utility.h"
#include "Logging.h"
#include "FileBlock.h"
#include <cstdlib>
#include "BinCodec.h"
#include "TAPCodec.h"

using namespace std;


AtomBasicCodec::AtomBasicCodec(Logging logging, TargetMachine targetMachine) :mDebugInfo(logging), mTargetMachine(targetMachine)
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
            int min_len = (int) (e.shortT.length() - 1);
            for (int l = min_len; l < e.fullT.length(); l++) {
                string a = e.fullT.substr(0, l) + ".";
                mTokenDictStr[a] = e;
            }
        }

        if (e.tokeniseInfo & ADD_40_FOR_START_OF_STATEMENT) {
            mTokenDictId[e.id1 + 0x40] = e;
        }

        
    }

    if (logging.verbose) {

        string prefix = "";

        cout << "\nKeywords for which no tokenisation shall be made if the keyword is followed by an alphabetic char:\n";
        
        for (int i = 0; i < mBBMTokens.size(); i++) {
            TokenEntry e = mBBMTokens[i];
            if (e.tokeniseInfo & CONDITIONAL_TOKENISATION) {
                cout << prefix << e.fullT;
                prefix = ", ";
            }
        }
        cout << "\n";

        cout << "\nKeywords for which middle of statement applies:\n";
        prefix = "";
        for (int i = 0; i < mBBMTokens.size(); i++) {
            TokenEntry e = mBBMTokens[i];
            if (e.tokeniseInfo & MIDDLE_OF_STATEMENT) {
                cout << prefix << e.fullT;
                prefix = ", ";
            }
        }
        cout << "\n";

        cout << "\nKeywords for which start of statement applies:\n";
        prefix = "";
        for (int i = 0; i < mBBMTokens.size(); i++) {
            TokenEntry e = mBBMTokens[i];
            if (e.tokeniseInfo & START_OF_STATEMENT) {
                cout << prefix << e.fullT;
                prefix = ", ";
            }
        }
        cout << "\n";

        cout << "\nKeywords for which no tokenisation shall be made of a following name token:\n";
        prefix = "";
        for (int i = 0; i < mBBMTokens.size(); i++) {
            TokenEntry e = mBBMTokens[i];
            if (e.tokeniseInfo & FN_OR_PROC_NAME) {
                cout << prefix << e.fullT;
                prefix = ", ";
            }
        }
        cout << "\n";

        cout << "\nKeywords for which a goto line could exist and then should be tokenised:\n";
        prefix = "";
        for (int i = 0; i < mBBMTokens.size(); i++) {
            TokenEntry e = mBBMTokens[i];
            if (e.tokeniseInfo & GOTO_LINE) {
                cout << prefix << e.fullT;
                prefix = ", ";
            }
        }
        cout << "\n";

        cout << "\nKeywords for which the rest of the line should NOT be tokenised:\n";
        prefix = "";
        for (int i = 0; i < mBBMTokens.size(); i++) {
            TokenEntry e = mBBMTokens[i];
            if (e.tokeniseInfo & STOP_TOKENISE) {
                cout << prefix << e.fullT;
                prefix = ", ";
            }
        }
        cout << "\n";

        cout << "\nKeywords for which 0x40 shall be added to the id for left hand assignment:\n";
        prefix = "";
        for (int i = 0; i < mBBMTokens.size(); i++) {
            TokenEntry e = mBBMTokens[i];
            if (e.tokeniseInfo & ADD_40_FOR_START_OF_STATEMENT) {
                cout << prefix << e.fullT;
                prefix = ", ";
            }
        }
        cout << "\n";
    }

}

bool AtomBasicCodec::detokenise(string& srcFile, string& dstFile)
{
    // Read source code bytes
    Bytes tokenised_program;
    string program;
    if (!readFileBytes(true, srcFile, tokenised_program, program)) {
        return false;
    }

    // Detokenise the bytes
    Bytes source_program;
    bool faulty_program_termination;
    if (!detokenise(program, tokenised_program, source_program, faulty_program_termination)) {
        cout << "Failed to detokenise program '" << program << "'\n";
        return false;
    }

    // Write the bytes to file
    if (!writeFileBytes(false, program, source_program, dstFile)) {
        cout << "Can't write detokenised bytes to file '" << dstFile << "'\n";
        return false;
    }

    return true;

}

bool AtomBasicCodec::detokenise(TapeFile& tapeFile, string& dstFile)
{
    Bytes tokenised_code;

    // Get bytes from Tape File
    BinCodec BIN_codec(mDebugInfo);
    if (!BIN_codec.encode(tapeFile, tapeFile.header, tokenised_code)) {
        return false;
    }
    mTargetMachine = tapeFile.header.targetMachine; // set target based on that of the tape file

    if (mDebugInfo.verbose)
        cout << "Detokenising '" << tapeFile.header.name << "' as " << _TARGET_MACHINE(mTargetMachine) << " BASIC\n";

    // Detokenise the bytes
    Bytes source_program;
    bool faulty_program_termination;
    string program = tapeFile.header.name;
    if (!detokenise(program, tokenised_code, source_program, faulty_program_termination)) {
        cout << "Failed to detokenise program '" << program << "'\n";
        return false;
    }

    // Write the bytes to file
    if (!writeFileBytes(false, program, source_program, dstFile)) {
        cout << "Can't write detokenised bytes to file '" << dstFile << "'\n";
        return false;
    }

    return true;
}

bool AtomBasicCodec::detokenise(string program, Bytes& tokenisedProgram, string& dstFile, bool &faultyTermination)
{
    // Detokenise the bytes
    Bytes source_program;
    if (tokenisedProgram.size() == 0 || !detokenise(program, tokenisedProgram, source_program, faultyTermination)) {
        return false;
    }

    // Write the bytes to file
    if (!writeFileBytes(false, program, source_program, dstFile)) {
        cout << "Can't write detokenised bytes to file '" << dstFile << "'\n";
        return false;
    }

    return true;
}

bool AtomBasicCodec::detokenise(string program, Bytes& tokenisedProgram, Bytes& SourceProgram, bool &faultyTermination)
{
    if (mDebugInfo.verbose)
        cout << "\nDetokenising program '" << program << "'...\n\n";

    if (mTargetMachine <= BBC_MASTER)
        return detokeniseBBM(program, tokenisedProgram, SourceProgram, faultyTermination);
    else
        return detokeniseAtom(program, tokenisedProgram, SourceProgram, faultyTermination);

    if (mDebugInfo.verbose && faultyTermination)
        cout << "Program '" << program << "' didn't terminate with 0xff!\n";

    if (mDebugInfo.verbose) {
        cout << "\nDone encoding program '" << program << " '...\n\n";
    }
}

bool AtomBasicCodec::detokeniseBBM(string program, Bytes& tokenisedProgram, Bytes& SourceProgram, bool &faultyTermination)
{
    faultyTermination = false;

    int read_bytes = 0;

    int tape_file_sz = (int) tokenisedProgram.size();


    int line_pos = -1;
    int line_no_low = 0;
    int line_no_high = 0;
    bool first_line = true;
    bool end_of_program = false;   
    int line_len;

    bool parsing_goto_line = false;
    int encoded_goto_line_cnt = -1;
    string goto_line_bytes = "";
    
    TokeniseState t_state = TOKENISATION;
    BytesIter t_code_iter = tokenisedProgram.begin();
    int line_no = -1;
    while (t_code_iter < tokenisedProgram .end()  && !end_of_program) {
        
        Byte b = *t_code_iter++;

        switch (t_state) {

            case START_OF_LINE:

                if (line_pos == 0) {
                    if (b == 0xff) {
                        end_of_program = true;
                        int n_non_ABC_bytes = tape_file_sz - read_bytes;
                        if (mDebugInfo.verbose && n_non_ABC_bytes > 0) {
                            faultyTermination = true;
                            if (mDebugInfo.verbose)
                                cout << "Program file '" << program << "' contains " << n_non_ABC_bytes <<
                                " extra bytes after end of program - skipping this data for BBC Micro BASIC file generation!\n";
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
                else { // if (line_pos == 2) {
                    line_len = b;
                    line_no = line_no_high * 256 + line_no_low;
                    stringstream ss;
                    ss << setw(5) << line_no;
                    string s = ss.str();
                    for (int i = 0; i < s.length(); SourceProgram.push_back((Byte)s[i++]));
                    t_state = TOKENISATION;
                }
                break;

            default:

                if (b == 0xd) {
                    t_state = START_OF_LINE;
                    line_pos = 0;
                    if (!first_line) {
                        SourceProgram.push_back(0xd);
                    }
                    first_line = false;
                }

                else {

                    // Check for keyword
                    TokenEntry keyword_entry;
                    string keyword = "";
                    bool keyword_detected = false;
                    if (mTokenDictId.find(b) != mTokenDictId.end()) {
                        keyword_entry = mTokenDictId[(int) b];
                        keyword = keyword_entry.fullT;
                        keyword_detected = true;
                    }

                    bool stop_further_processing = false;

                    // Check for pending goto line no
                    if (t_state == PARSING_GOTO_LINE_NO) {
                        stop_further_processing = true;
                        if (goto_line_bytes.length() == 0) {
                            if (b == 0x20)
                                SourceProgram.push_back(b);
                            else if (b == 0x8d)
                                goto_line_bytes += (char)b;
                            else { // Not an encoded line no => continue processing!
                                t_state = TOKENISATION;
                                stop_further_processing = false;
                            }
                        }
                        else { // some encoded bytes already read

                            goto_line_bytes += (char)b;
                            if (goto_line_bytes.length() == 4) {

                                int goto_line;
                                if (!decodeLineNo(goto_line_bytes, goto_line))
                                    return false;
                                stringstream ss;
                                ss << goto_line;
                                string s = ss.str();
                                for (int i = 0; i < s.length(); SourceProgram.push_back((Byte)s[i++]));
                                t_state = TOKENISATION;
                            }
                        }
                    }

                    // No further tokenisation?
                    if (!stop_further_processing && t_state == NO_TOKENISATION) {
                        SourceProgram.push_back(b);
                    }

                    if (!stop_further_processing && t_state == TOKENISATION) {
                        if (keyword_detected) {
                            // Add keyword to source code
                            for (int i = 0; i < keyword.length(); SourceProgram.push_back((Byte)keyword[i++]));
                            // Check for GOTO-type of keyword
                            if (keyword_entry.tokeniseInfo & TokeniseInfo::GOTO_LINE) {
                                t_state = PARSING_GOTO_LINE_NO;
                                goto_line_bytes = "";
                            }
                            // Check for stop tokenisation-type of keyword
                            else if (keyword_entry.tokeniseInfo & TokeniseInfo::STOP_TOKENISE)
                                t_state = NO_TOKENISATION;
                        }
                        else
                            // No (tokenised) keyword => add char to source code 'as is'
                            SourceProgram.push_back(b);
                    }

                }

                break;
        }

    }

    faultyTermination = faultyTermination || !end_of_program;


    return true;
}

bool AtomBasicCodec::detokeniseAtom(string program, Bytes& tokenisedProgram, Bytes& SourceProgram, bool& faultyTermination)
{

    int read_bytes = 0;

    int tape_file_sz = (int) tokenisedProgram.size();

    int line_pos = -1;
    int line_no_high = 0;
    bool first_line = true;
    bool end_of_program = false;

    BytesIter t_code_iter = tokenisedProgram.begin();
    while (t_code_iter < tokenisedProgram.end() && !end_of_program) {

        Byte b = *t_code_iter++;

        //
        // Format of Acorn Atom BASIC program in memory:
        // {<cr> <linehi> <linelo> <text>} <cr> <ff>
        //
        
            if (line_pos == 0) {

                if (b == 0xff) {
                    end_of_program = true;
                    int n_non_ABC_bytes = tape_file_sz - read_bytes;
                    if (mDebugInfo.verbose)
                        cout << "Program file '" << program << "' contains " << n_non_ABC_bytes <<
                        " extra bytes after end of program - skipping this data for Acorn Atom BASIC file generation!\n";
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
                stringstream ss;
                ss << setw(5) << line_no;
                string s = ss.str();
                for (int i = 0; i < s.length(); SourceProgram.push_back((Byte)s[i++]));
            }
            else if (b == 0xd) {
                line_pos = 0;
                if (!first_line) {
                    SourceProgram.push_back(0xd);
                }
                first_line = false;

            }
            else {
                SourceProgram.push_back(b);
            }




    }

    faultyTermination = faultyTermination || !end_of_program;

    return true;
}

bool AtomBasicCodec::readFileBytes(bool tokenisedBytes, string& srcFile, Bytes& data, string& program)
{
    ios_base::openmode flags = ios::in;
    if (tokenisedBytes)
        flags = ios::in | ios::binary | ios::ate;

    ifstream fin(srcFile, flags);
    if (!fin) {
        cout << "Failed to open file '" << srcFile << "´\n";
        return false;
    }
    filesystem::path fin_p = srcFile;
    string src_file_name = fin_p.stem().string();
    program = TapeFile::crValidBlockName(mTargetMachine, src_file_name);

    // Read source code bytes
    Byte b;
    if (tokenisedBytes) {
        while (fin.read((char*)&b, sizeof(Byte)))
            data.push_back(b);
    }
    else {
        string line;
        while (getline(fin, line)) {
            for (int i = 0; i < line.length(); i++)
                data.push_back((Byte)line[i]);
            data.push_back(0xd);
        }
    }
    fin.close();

    return true;
}

bool AtomBasicCodec::writeFileBytes(bool tokenisedBytes, string program, Bytes& data, string& dstFile)
{
    if (data.size() == 0)
        return false;

    ios_base::openmode flags = ios::out;
    if (tokenisedBytes)
        flags = ios::out | ios::binary | ios::ate;

    // Write the bytes to file
    ofstream fout(dstFile, flags);
    if (!fout) {
        cout << "Can't write to file '" << dstFile << "'\n";
        return false;
    }
    BytesIter bi = data.begin();
    if (tokenisedBytes) {
        while (bi < data.end())
            fout << *bi++;
    }
    else {
        while (bi < data.end()) {
            if (*bi == 0xd)
                fout << "\n";
            else
                fout << *bi;
            bi++;
        }
    }

    fout.close();

    return true;
}

bool AtomBasicCodec::tokenise(string& srcFile, string& dstFile)
{
    Bytes src_code_bytes;
    string program;
    if (!readFileBytes(false, srcFile, src_code_bytes, program)) {
        cout << "Failed to read bytes from file '" << srcFile << "´\n";
        return false;
    }

    // Tokenise the bytes
    Bytes tokenised_bytes;
    if (!tokenise(program, src_code_bytes, tokenised_bytes)) {
        return false;
    }

    if (!writeFileBytes(true, program, tokenised_bytes, dstFile)) {
        cout << "Can't write bytes to file '" << dstFile << "'\n";
        return false;
    }

    return true;

}

bool AtomBasicCodec::tokenise(string& srcFile, TapeFile& tapeFile)
{
    Bytes src_code_bytes;
    string program;
    if (!readFileBytes(false, srcFile, src_code_bytes, program)) {
        cout << "Failed to read bytes from file '" << srcFile << "´\n";
        return false;
    }

    // Tokenise the bytes
    Bytes tokenised_bytes;
    if (!tokenise(program, src_code_bytes, tokenised_bytes)) {
        return false;
    }

    // Encode the tokenised bytes as a Tape File
    FileHeader file_header(mTargetMachine, program);
    BinCodec BIN_codec(mDebugInfo);
    if (!BIN_codec.decode(file_header, tokenised_bytes, tapeFile)) {
        cout << "Failed to encode tokenised bytes as a Tape File for program '" << program << "´\n";
        return false;
    }

    return true;
}

bool AtomBasicCodec::tokenise(string& srcFile, Bytes& tokenisedProgram)
{
    // Read source code bytes
    Bytes source_code;
    string program;
    if (!readFileBytes(false, srcFile, source_code, program)) {
        return false;
    }

    // Tokenise them
    if (!tokenise(program, source_code, tokenisedProgram)) {
        return false;
    }

    return true;
}

bool AtomBasicCodec::tokenise(string program, Bytes& sourceCode, Bytes& tokenisedData)
{

    if (sourceCode.size() == 0)
        return false;

    // Tokenise each line
    BytesIter bi = sourceCode.begin();
    while (bi < sourceCode.end()) {

        // Read line
        string line;
        string code ;
        int line_no;
        while (bi < sourceCode.end() && *bi != 0xd)
            line += (char)*bi++;
        if (bi < sourceCode.end())
            bi++;
        istringstream sin(line);

        // Get line no
        sin >> line_no;

        // Get rest of line
        getline(sin, code);

        // Add start of line (0xd) and line no
        tokenisedData.push_back(0xd);
        tokenisedData.push_back(line_no / 256);
        tokenisedData.push_back(line_no % 256);  

        if (mTargetMachine <= BBC_MASTER) {
            string tCode;
            if (!tokeniseLine(line_no, code, tCode)) {
                cout << "Failed to tokenise line '" << code << "' ('" << tCode << "')\n";
                return false;
            }
            tokenisedData.push_back((Byte) (4+tCode.length())); // line length + "size for <CR>,line no & line no"=4
            for (int i = 0; i < tCode.length(); tokenisedData.push_back((Byte) tCode[i++]));
        }
        else { // mTargetMachine == ACORN_ATOM
            for (int i = 0; i < code.length(); tokenisedData.push_back((Byte) code[i++]));
        }
    }

    // Add end of program marker
    tokenisedData.push_back(0x0d);
    tokenisedData.push_back(0xff);

    return true;

}

bool AtomBasicCodec::tokeniseLine(int lineNo, string &line, string& tCode)
{
    string space, token;
    TokenEntry entry;
    int pos = 0;
    bool start_of_statement = true;
    bool within_string = false;
    bool fun_or_proc = false;
    string code = line;
    bool no_tokenisation = false;
    uint8_t last_keyword_id;
    string last_space;
    bool pending_conditional_tokenisation = false;
    string last_token;

    bool pending_goto = false;

    tCode = "";
    while (code.length() > 0) {

        bool skip_further_processing = false;

        // Get next token
        if (!getKeyWord(fun_or_proc, start_of_statement, within_string, code, space, token, entry)) {
            return false;
        }
        bool keyword_detected = entry.fullT != "" && !no_tokenisation;

        // Check for any pending conditional tokenisation of a keyword (from the last round)
        if (pending_conditional_tokenisation) {
            // Decide whether the last keyword should be tokenised or not
            pending_conditional_tokenisation = false;
            if (
                space.size() == 0 && token.size() > 0 &&
                ((token[0] >= 'a' && token[0] <= 'z') || (token[0] >= 'A' && token[0] <= 'Z'))
            ) {
                // Don't tokenise as followed by an alphabetic character
                tCode += last_space + last_token;
                //cout << "CONDITIONAL TOKENISATION at line " << lineNo << " of '" << last_token << "'" <<
                //    " => NO TOKENISATION as followed by '" << space + token << "'\n";
            }
            else {
                tCode += last_space + (char)last_keyword_id;
                //cout << "CONDITIONAL TOKENISATION at line " << lineNo << " of '" << last_token << "'" <<
                //    " => TOKENISATION as followed by '" << space + token << "'\n";
            }
        }

        // If the token was a keyword, then determine it's id
        uint8_t keyword_id = 0x0;
        if (keyword_detected) {
            if ((entry.tokeniseInfo & ADD_40_FOR_START_OF_STATEMENT) && start_of_statement) {
                keyword_id = entry.id1 + 0x40; // start of a statement <=> potential left side of assigment => add 0x40 to id
                //cout << "ASSIGMENT at line " << lineNo << " of '" << last_token << "'" <<
                //    " => add 0x40 to keyword id => 0x" << hex << keyword_id << " (instead of 0x " << entry.id1 << dec << ")\n";
            }
            else {
                keyword_id = entry.id1;
            }
        }

        // Check for any pending GOTO line no to encode (from the last round)
        if (pending_goto) {
            pending_goto = false;
            int line_no;
            if (entry.fullT == "" && token2Int(token, line_no) && line_no >= 0 && line_no < 32768) {
                // A GOTO line no that shall be encoded identified           
                string encoded_bytes;
                if (!encodeLineNo(line_no, encoded_bytes))
                    return false;
                tCode += space + encoded_bytes;
                skip_further_processing = true; // stop this round
                // cout << "GOTO line " << line_no << " to encode for keyword '" << last_token << "' at line " << lineNo << "\n";
            }
            //cout << "NO GOTO LINE (but instead a token '" << token << "') to encode for keyword '" << last_token << "' at line " << lineNo << "\n";
            //if (keyword_detected)
            //    cout << "KEYWORD detected at line " << lineNo << "\n";
        }

        // Process a tokenisable keyword
        if (!skip_further_processing && !no_tokenisation && keyword_detected) {

            if (entry.tokeniseInfo & TokeniseInfo::CONDITIONAL_TOKENISATION) {
                // The tokenisation of the keyword is conditional and has to wait to next round (so save it)
                pending_conditional_tokenisation = true;
            }
            else {
                tCode += space + (char) keyword_id;
                if (entry.tokeniseInfo & TokeniseInfo::GOTO_LINE) {
                    // Postpone processing of GOTO line no to next round
                    pending_goto = true;
                    // cout << "GOTO-type of keyword '" << entry.fullT << "' at line " << lineNo << "\n";
                }
                else if (entry.tokeniseInfo & TokeniseInfo::STOP_TOKENISE) {
                    // No further tokenisation shall be made for the line
                    no_tokenisation = true;
                    // cout << "Stop tokenisation - type of keyword '" << entry.fullT << "' at line " << lineNo << "\n";
                }
            }
        }
        
        // Process a non-keyword token (including non-tokenisable keyword)
        if (!skip_further_processing && !keyword_detected) {

            // As there are keywords with different ids for assignment and use in expression,
            // we need to know if it's an assignment (start of statement) or not.
            if (token == ":")                  
                start_of_statement = true;
            else
                start_of_statement = false;

            // Stop tokenisation if we're in the middle of a string
            if (token == "\"")
                within_string = !within_string;

            tCode += space + token;
        }

        if (keyword_detected)
            last_keyword_id = keyword_id; //remember keyword id for next round

        last_space = space;
        last_token = token;

    }

    // Catch a potentially last pending keyword
    if (pending_conditional_tokenisation) {
        // Decide whether the last keyword should be tokenised or not
        pending_conditional_tokenisation = false;
       tCode += last_space + (char)last_keyword_id;
    }

    return true;
}

bool AtomBasicCodec::isDelimiter(char c)
{
    const string delimiters = "'+-*/^!,<>?;[]:*{}@\"#$%&'()=~|_£";

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
    const TokenEntry empty = { "", "", 0x00, 0x00 };
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

// Tokenising the line number:
    // The top two bits are split off each of the two bytes of the 16-bit line number.
    // These bits are combined (in binary as 00LlHh00), exclusive-ORred with 0x54, and
    // stored as the first byte of the 3-byte sequence. The remaining six bits of each
    // byte are then stored, in LO/HI order, ORred with 0x40.
    // 
    // Used for AUTO, DELETE, ELSE, GOSUB, GOTO, LIST, RENUMBER, RESTORE and THEN
    // 
    // Only seems to be applied for line no < 32768 (0x4000)
    //
bool AtomBasicCodec::encodeLineNo(int lineNo, string &encodedBytes)
{
    if (lineNo < 0 || lineNo >= 32768)
        return false;

    encodedBytes = "\x8d";
    int b15b14 = (lineNo >> 14) & 0x3;
    int b7b6 = (lineNo >> 6) & 0x3;
    int byte_0 = (((b7b6 << 4) & 0x30) | ((b15b14 << 2) & 0x0c)) ^ 0x54;
    int byte_1 = (lineNo & 0x3f) | 0x40;
    int byte_2 = ((lineNo >> 8) & 0x3f) | 0x40;
    encodedBytes += (char) byte_0;
    encodedBytes += (char) byte_1;
    encodedBytes += (char) byte_2;
    return true;
}

bool AtomBasicCodec::decodeLineNo(string encodedBytes, int& lineNo)
{

    if (encodedBytes[0] != (char) 0x8d)
        return false;

    int b6b0 = encodedBytes[2] & 0x3f;
    int b13b8 = encodedBytes[3] & 0x3f;
    int b7b6 = ((encodedBytes[1] ^ 0x54) >> 4) & 0x3;
    int b15b14 = ((encodedBytes[1] ^ 0x54) >> 2) & 0x3;
    lineNo = (b15b14 << 14) | (b13b8 << 8) | (b7b6 << 6) | b6b0;
    return true;
}

bool AtomBasicCodec::token2Int(string token, int &num)
{
    try {
        num = stoi(token);
    }
    catch (const invalid_argument) {
        return false;
    }

    return true;
}
