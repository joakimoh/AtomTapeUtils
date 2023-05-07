#include "Utility.h"
#include "CommonTypes.h"
#include <iostream>
#include <sstream>
#include "Debug.h"
#include <filesystem>

string crDefaultOutFileName(string filePath, string fileExt)
{
    filesystem::path file_path = filePath;
    filesystem::path dir_path = file_path.parent_path();
    filesystem::path file_base = file_path.stem();

    string file_ext = file_path.extension().string();

    filesystem::path out_file_name;
    if (fileExt.size() == 0)
        out_file_name = file_base.string();
    else
        out_file_name = file_base.string() + "." + fileExt;

    filesystem::path file_out_path = dir_path / out_file_name;

    return file_out_path.string();
}
string crDefaultOutFileName(string filePath)
{
    filesystem::path file_path = filePath;
    filesystem::path dir_path = file_path.parent_path();
    filesystem::path file_base = file_path.stem();

    string file_ext = file_path.extension().string();

    filesystem::path out_file_name = file_base.string() + "_out" + file_ext;

    filesystem::path file_out_path = dir_path / out_file_name;

    return file_out_path.string();
}
string crEncodedFileNamefromDir(string dirPath, TAPFile mTapFile, string fileExt)
{
    filesystem::path dir_path = dirPath;
    filesystem::path file_base = mTapFile.validFileName;

    string suffix;
    string file_ext;
    if (fileExt.size() == 0)
        file_ext = "";
    else
        file_ext = "." + fileExt;

    if (mTapFile.complete)
        suffix = file_ext;
    else
        suffix = "_incomplete_" + to_string(mTapFile.firstBlock) + "_" + to_string(mTapFile.lastBlock) + file_ext;

    filesystem::path output_file_name = dir_path / file_base;
    output_file_name += suffix;

    return output_file_name.string();;
}


/*
* Parse time on format mm:ss.ss
*/
double decodeTime(string time)
{
    stringstream ts(time);
    double hour, min, sec;
    char c;

    ts >> hour >> c >> min >> c >> sec;

    return (hour * 3600 + min * 60 + sec);

}

string encodeTime(double t) {
    char t_str[64];
    int t_h = (int)trunc(t / 3600);
    int t_m = (int)trunc((t - t_h * 3600) / 60);
    double t_s = t - t_h * 3600 - t_m * 60;
    sprintf(t_str, "%dh:%dm:%.6fs (%fs)", t_h, t_m, t_s, t);
    return string(t_str);
}

string parseFileName(char *blockFileName)
{
    string s = "";

    for (int i = 0; i < 14 && blockFileName[i] != 0xd; i++)
        s += blockFileName[i];

    return s;
}

char digitToHex(int d)
{
    if (d >= 0 && d <= 9)
        return (char)(d + int('0'));
    else if (d >= 10 && d <= 15)
        return (char)(d - 10 + int('a'));
    else
        return 'X';
}
/*
* Create a valid Atom Tape Block Name from a DOS/Linux/MacOs filename.
* 
* For Atom a filename can contain any printable character (no limitation)
* but cannot be longer than 13 characters in total.
*
* Each occurrence of _hh where hh is a hex number is assumed to be an escape
* sequence and will be replaced by an ASCII character with hex value hh.
* 
* A filename longer than 13 characters will be truncated to 13 characters.
* 
*/
string blockNameFromFilename(string fn)
{
    string s = "";
    int len = (int)fn.length();
    int p = 0;
    int atom_file_pos = 0;
    while (p < len && atom_file_pos < 13) {
        char c = fn[p];
        // Look for escape sequences __ and _hh 
        if (fn[p] == '_') {
            if (len - p >= 1 && fn[p + 1] == '_') { // '--' => '_'
                c = '_';
                p++;
            }
            else if (len - p >= 2 && isxdigit(fn[p + 1]) && isxdigit(fn[p + 2])) { // '_hh' => ASCII hh
                string hs = fn.substr(p + 1, 2);
                int h;
                sscanf(hs.c_str(), "%2x", &h);
                c = (char) h;
                p += 2;
            }
            else { // ERROR
                return "";
            }
        }
        s += c;
        atom_file_pos++;
        p++;
        
    }

    return s;
}

/*
* Create a valid DOS/Linux/MacOs filename from an Acorn Atom tape filename.
* 
* Characters that are not allowed in a DOS/Linux/MacOs filename are:
* /<>:"\|?* and ASCII 0-31
* 
* '_' is used as an escape sequence (_hh where hh is the ASCII in hex)
* and will therefore not either be allowed in the filename chosen.
*
* Filenames cannot either end in SPACE or DOT.*/
string filenameFromBlockName(string fileName)
{
    string invalid_chars = "\"/<>:\|?*";
    string s = "";
    for (auto& c : fileName) {
        if (c != '_' && invalid_chars.find(c) == string::npos && c >= ' ')
             s += c;
        else if (c == '_')
            s += "__";
        else {
            s += "_";
            s += digitToHex((int)c / 16);
            s += digitToHex((int)c % 16);
        }
    }

    return s;
}

void logData(int address, BytesIter data_iter, int data_sz) {

    if (DEBUG_LEVEL != DBG)
        return;

    BytesIter di = data_iter;
    BytesIter di_16 = data_iter;
    int a = address;
    bool new_line = true;
    int line_sz;

    for (int i = 0; i < data_sz; i++) {
        if (new_line) {   
            new_line = false;
            line_sz = (i > data_sz - 16 ? data_sz - i : 16);
            printf("\n%.4x ", a);
        }
        printf("%.2x ", *di++);

        if (i % 16 == line_sz - 1) {
            new_line = true;
            for (int k = 0; k < 16 - line_sz; k++)
                printf("   ");
            printf(" ");
            for (int j = i; j < i + line_sz; j++) {
                if (*di_16 >= 0x20 && *di_16 <= 0x7e)
                    printf("%c", *di_16);
                else
                    printf(".");
                di_16++;
            }
            a += line_sz;
            di_16 = di;
        }

    }
    printf("\n");
}

BlockType parseBlockFlag(AtomTapeBlockHdr hdr, int& blockLen) {

    BlockType type;


    if ((hdr.flags & 0x80) == 0x80) { // b7 = 1 => Not the last block (i.e., first or middle block)
        if ((hdr.flags & 0x20) == 0x20) // b5 = 1 => Not the first block (i.e. middle block)
            type = BlockType::Other;
        else
            type = BlockType::First;

    }
    else { // The last block (could also be the first block if there is only one block in total
        if ((hdr.flags & 0x20) == 0x20) // b5 = 1 => Not the first block
            type = BlockType::Last;
        else
            type = BlockType::Single; // Single block which is both the First and Last block
    }

    if ((hdr.flags & 0x40) == 0x40) // b5 = 1 => block contains data
        blockLen = hdr.dataLenM1 + 1;
    else
        blockLen = 0;

    return type;

}

//
// Extract parameters from the read block's header.
//
// If the header was not completely read, then only the parameters
// that were part of the read part of the block will be extracted.
// Non-read parameters will be initialised to -1/false/???.
//
// Returns true if all parameters were successfully extracted (i.e.
// a complete block header was read)
//
//
// The Atom Tape Block has a sequential structure according to below:
//        uint8_t preamble[4];
//        char name[2:14]; // 1 to 13 characters terminated with 0xd
//        Byte flags;
//        Byte blockNoHigh;
//        Byte blockNoLow;
//        Byte dataLenM1;
//        Byte execAdrHigh;
//        Byte execAdrLow;
//        Byte loadAdrHigh;
//        Byte loadAdrLow;
//
// The read block is stored in a Wouter Ras ATM header format structure (ATMBlock).
// This structure is similar to the tape structure but neither includes the block no
// nor the flags. This is most likely because the block no (and related flag bits)
// follows from the block sequence itself as blocks are numbered 0, 1, 2, ..., n-1
// where n = no of blocks..
//
bool extractBlockPars(
    ATMBlock block, BlockType blockType, int nReadChars,
    int& loadAdr, int& loadAdrUB, int& execAdr, int& blockSz,
    bool& isAbcProgram, string& fileName
)
{
    loadAdr = -1;
    loadAdrUB = -1;
    execAdr = -1;
    blockSz = -1;
    isAbcProgram = false;
    fileName = "???";

    // Get TAP filename if it was completey read
    if (nReadChars > 4) { // name comes after 4-bytes preamble
        fileName = "";
        int sz = 0;
        for (sz = 0; sz < 13 && block.hdr.name[sz] != 0x0; sz++) {
            if (block.hdr.name[sz] < 0x20 || block.hdr.name[sz] > 0x7f) {
                fileName += "?";
                break;
            }
            else
                fileName += block.hdr.name[sz];
        }
    } else
        return false; // If filename not read then the rest will not have been read either...

    // Calculate offset to the rest of the block's parameters (that comes after the preamble and filename)
    int offset = 4 + fileName.length();

    if (nReadChars > offset + (long)  & ((AtomTapeBlockHdr*)0)->loadAdrLow)
        loadAdr = block.hdr.loadAdrHigh * 256 + block.hdr.loadAdrLow;

    if (nReadChars > offset + (long)&((AtomTapeBlockHdr*)0)->dataLenM1)
        blockSz = block.hdr.lenHigh * 256 + block.hdr.lenLow;

    if (loadAdr > 0 && blockSz > 0)
        loadAdrUB = loadAdr + blockSz;

    if (nReadChars > offset + (long) &((AtomTapeBlockHdr*)0) -> execAdrLow)
        execAdr = block.hdr.execAdrHigh * 256 + block.hdr.execAdrLow;

    if (execAdr == 0xc2b2)
        isAbcProgram = true;

    return (nReadChars >= sizeof(AtomTapeBlockHdr) + offset);
}
