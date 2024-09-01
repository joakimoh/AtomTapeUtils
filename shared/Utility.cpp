#include "Utility.h"
#include "CommonTypes.h"
#include <iostream>
#include <sstream>
#include "Debug.h"
#include <filesystem>
#include "WaveSampleTypes.h"


void Utility::logTapeTiming(TapeProperties tapeTiming)
{
    cout << "Base frequency: " << dec << tapeTiming.baseFreq << " Hz\n";
    cout << "Baudrate: " << dec << tapeTiming.baudRate << " Hz\n";
    cout << "\n";
    cout << "Min block gap: " << dec << tapeTiming.minBlockTiming.blockGap << " s\n";
    cout << "Min first block gap: " << dec << tapeTiming.minBlockTiming.firstBlockGap << " s\n";
    cout << "Min first lead tone: " << dec << tapeTiming.minBlockTiming.firstBlockLeadToneDuration << " s\n";
    cout << "Min last block gap: " << dec << tapeTiming.minBlockTiming.lastBlockGap << " s\n";
    cout << "Min micro tone: " << dec << tapeTiming.minBlockTiming.microLeadToneDuration << " s\n";
    cout << "Min other block lead tone: " << dec << tapeTiming.minBlockTiming.otherBlockLeadToneDuration << " s\n";
    cout << "Min trailer tone: " << dec << tapeTiming.minBlockTiming.trailerToneDuration << " s\n";
    cout << "\n";
    cout << "Nom block gap: " << dec << tapeTiming.minBlockTiming.blockGap << " s\n";
    cout << "Nom first block gap: " << dec << tapeTiming.nomBlockTiming.firstBlockGap << " s\n";
    cout << "Nom first lead tone: " << dec << tapeTiming.nomBlockTiming.firstBlockLeadToneDuration << " s\n";
    cout << "Nom last block gap: " << dec << tapeTiming.nomBlockTiming.lastBlockGap << " s\n";
    cout << "Nom micro tone: " << dec << tapeTiming.nomBlockTiming.microLeadToneDuration << " s\n";
    cout << "Nom other block lead tone: " << dec << tapeTiming.nomBlockTiming.otherBlockLeadToneDuration << " s\n";
    cout << "Nom trailer tone: " << dec << tapeTiming.nomBlockTiming.trailerToneDuration << " s\n";
    cout << "\n";
    cout << "Phaseshift: " << dec << tapeTiming.phaseShift << " degrees\n";
    cout << "Preserve timing: " << dec << tapeTiming.preserve << "\n";
}
bool Utility::setBitTiming(int baudRate, TargetMachine targetMachine, int & startBitCycles, int & lowDataBitCycles, int & highDataBitCycles, int & stopBitCycles)
{
    if (baudRate == 300) {
        startBitCycles = 4; // Start bit length in cycles of F1 frequency carrier
        lowDataBitCycles = 4; // Data bit length in cycles of F1 frequency carrier
        highDataBitCycles = 8; // Data bit length in cycles of F2 frequency carrier
        if (targetMachine)
            stopBitCycles = 8; // Stop bit length in cycles of F2 frequency carrier
        else
            stopBitCycles = 9;
    }
    else if (baudRate == 1200) {
        startBitCycles = 1; // Start bit length in cycles of F1 frequency carrier
        lowDataBitCycles = 1; // Data bit length in cycles of F1 frequency carrier
        highDataBitCycles = 2; // Data bit length in cycles of F2 frequency carrier
        if (targetMachine)
            stopBitCycles = 2; // Stop bit length in cycles of F2 frequency carrier
        else
            stopBitCycles = 3;
    }
    else {
        cout << "Invalid baud rate " << baudRate << "!\n";
        return false;
    }

    return true;
}

string Utility::roundedPD(string prefix, double d, string suffix)
{
    ostringstream s;
    if (d > 0)
        s << setw(4) << setprecision(2) << d << setw(1) << "s";
    else
        s << setw(5) << "-";

    return prefix + s.str()  + suffix;
}

void Utility::logTAPFileHdr(TapeFile& tapeFile)
{
    Utility::logTAPFileHdr(&cout, tapeFile);
}

void Utility::logTAPFileHdr(ostream* fout, TapeFile& tapeFile)
{
    if (fout == NULL)
        return;

    FileBlock& block = tapeFile.blocks[0];

    if (tapeFile.fileType == ACORN_ATOM) {    
        uint32_t exec_adr = block.atomHdr.execAdrHigh * 256 + block.atomHdr.execAdrLow;
        uint32_t load_adr = block.atomHdr.loadAdrHigh * 256 + block.atomHdr.loadAdrLow;
        uint32_t n_blocks = tapeFile.blocks.size();
        uint32_t file_sz = 0;
        for (int i = 0; i < n_blocks; i++)
            file_sz += tapeFile.blocks[i].atomHdr.lenHigh * 256 + tapeFile.blocks[i].atomHdr.lenLow;
        *fout << "\n" << setw(13) << block.atomHdr.name << " " << hex << setw(4) << load_adr << " " <<
            load_adr + file_sz - 1 << " " << exec_adr << " " << n_blocks << "\n";
    }
    else if (tapeFile.fileType <= BBC_MASTER) {
        uint32_t exec_adr = Utility::bytes2uint(&block.bbmHdr.execAdr[0], 4, true);
        uint32_t load_adr = Utility::bytes2uint(&block.bbmHdr.loadAdr[0], 4, true);
        uint32_t block_no = Utility::bytes2uint(&block.bbmHdr.blockNo[0], 2, true);
        uint32_t file_sz = 0;
        uint32_t n_blocks = tapeFile.blocks.size();
        for(int i = 0; i < n_blocks;i++)
            file_sz += Utility::bytes2uint(&tapeFile.blocks[i].bbmHdr.blockLen[0], 2, true);
        *fout << "\n" << setw(10) << block.bbmHdr.name << " " << hex << setw(8) << load_adr << " " <<  load_adr + file_sz - 1<<  " " << exec_adr << " " <<
            setw(4) << dec << n_blocks << " " << 
            setw(4) << setfill('0') << hex << file_sz << "\n" << setfill(' ');
    }
    else {
    }
}

void Utility::logTAPBlockHdr(FileBlock& block, uint32_t adr_offset, uint32_t blockNo)
{
    Utility::logTAPBlockHdr(&cout, block, adr_offset);
}

void Utility::logTAPBlockHdr(ostream* fout, FileBlock& block, uint32_t adr_offset, uint32_t blockNo /* Atom only*/)
{
    if (fout == NULL)
        return;

    if (block.blockType == ACORN_ATOM) {
        uint32_t exec_adr = block.atomHdr.execAdrHigh * 256 + block.atomHdr.execAdrLow;
        uint32_t load_adr = block.atomHdr.loadAdrHigh * 256 + block.atomHdr.loadAdrLow;
        uint32_t block_sz = block.atomHdr.lenHigh * 256 + block.atomHdr.lenLow;
        *fout << setw(13) << setfill(' ') << block.atomHdr.name << " " <<
            hex << setfill('0') << setw(4) << load_adr << " " <<
            hex << setfill('0') << setw(4) << load_adr + block_sz - 1 << " " <<
            hex << setfill('0') << setw(4) << exec_adr << " " <<
            dec << setfill(' ') << setw(4) << dec << blockNo << " " <<
            hex << setfill('0') << setw(4) << block_sz << " " << setfill(' ');
        if (block.leadToneCycles != -1 || block.microToneCycles != -1 || block.blockGap != -1) {
            *fout << Utility::roundedPD(" LEAD TONE ", (double)block.leadToneCycles / F2_FREQ, ": ") <<
                Utility::roundedPD("MICRO TONE ", (double)block.microToneCycles / F2_FREQ, ": ") <<
                Utility::roundedPD("GAP ", block.blockGap, "");
        }
        *fout <<    "\n";
    }
    else {
        uint32_t exec_adr = Utility::bytes2uint(&block.bbmHdr.execAdr[0], 4, true);
        uint32_t file_load_adr = Utility::bytes2uint(&block.bbmHdr.loadAdr[0], 4, true);
        uint32_t block_sz = Utility::bytes2uint(&block.bbmHdr.blockLen[0], 2, true);
        uint32_t block_no = Utility::bytes2uint(&block.bbmHdr.blockNo[0], 2, true);
        Byte flag = block.bbmHdr.blockFlag;
        *fout << setw(10) << setfill(' ') << block.bbmHdr.name << " " <<
            hex << setfill('0') << setw(8) << file_load_adr + adr_offset << " " <<
            hex << setfill('0') << setw(8) << file_load_adr + adr_offset + block_sz - 1 << " " <<
            hex << setfill('0') << setw(8) << exec_adr << " " <<
            dec << setfill(' ') << setw(4) << dec << block_no << " " <<
            hex << setfill('0') << setw(4) << block_sz << " " <<
            hex << setfill('0') << setw(2) << hex << (int)flag << setfill(' ');
        if (block.leadToneCycles != -1 || block.trailerToneCycles != -1 || block.blockGap != -1) {
            *fout << 
                " PRELUDE LEAD " << setw(5) << (block.preludeToneCycles>0?to_string(block.preludeToneCycles):"-") << " cycles: " <<
                Utility::roundedPD(" (POSTLUDE) LEAD ", (double)block.leadToneCycles / F2_FREQ, ": ") <<
                Utility::roundedPD(" TRAILER ", (double)block.trailerToneCycles / F2_FREQ, ": ") <<
                Utility::roundedPD("GAP ", block.blockGap, "");
        }
         *fout <<   "\n";
    }

}



void Utility::logAtomTapeHdr(AtomTapeBlockHdr &hdr, char blockName[])
{
    return Utility::logAtomTapeHdr(&cout, hdr, blockName);
}

void Utility::logBBMTapeHdr(BBMTapeBlockHdr& hdr, char blockName[])
{
    return Utility::logBBMTapeHdr(&cout, hdr, blockName);
}

void Utility::logBBMTapeHdr(ostream* fout, BBMTapeBlockHdr& hdr, char blockName[])
{
    uint32_t exec_adr = Utility::bytes2uint(&hdr.execAdr[0], 4, true);
    uint32_t load_adr = Utility::bytes2uint(&hdr.loadAdr[0], 4, true);
    uint32_t block_sz = Utility::bytes2uint(&hdr.blockLen[0], 2, true);
    uint32_t block_no = Utility::bytes2uint(&hdr.blockNo[0], 2, true);
    string block_name;
    BlockType block_type = BlockType::Other;
    if (hdr.blockFlag & 0x80) {
        if (block_no == 0)
            block_type = BlockType::Single;
        else
            block_type = BlockType::Last;
    }
    else if (block_no == 0)
        block_type = BlockType::First;
    else
        block_type = BlockType::Other;

    for (int i = 0; i < 13 && blockName[i] != 0x0; block_name += blockName[i++]);
    *fout << setfill(' ') << setw(10) << block_name << " " <<
        hex << setfill('0') << setw(8) << load_adr << " " <<
        hex << setfill('0') << setw(8) << load_adr + block_sz - 1 << " " <<
        hex << setfill('0') << setw(8) << exec_adr << " " <<
        dec << setfill(' ') << setw(4) << dec << block_no << " " <<
        hex << setfill('0') << setw(4) << block_sz << " " <<
        hex << setfill(' ') << setw(8) << _BLOCK_ORDER(block_type) << " " <<
        "\n";
}
void Utility::logAtomTapeHdr(ostream* fout, AtomTapeBlockHdr &hdr, char blockName[])
{
    uint32_t exec_adr = hdr.execAdrHigh * 256 + hdr.execAdrLow;
    uint32_t load_adr = hdr.loadAdrHigh * 256 + hdr.loadAdrLow;
    int block_sz;
    uint32_t block_no = hdr.blockNoHigh * 256 + hdr.blockNoLow;
    string block_name;
    int len;
    BlockType block_type = Utility::parseAtomTapeBlockFlag(hdr, block_sz);
    for (int i = 0; i < 13 && blockName[i] != 0xd; block_name += blockName[i++]);
    *fout << setfill(' ') << setw(13)  << block_name << " " <<
        hex << setfill('0') << setw(4) << load_adr << " " <<
        hex << setfill('0') << setw(4) << load_adr + block_sz - 1 << " " <<
        hex << setfill('0') << setw(4) << exec_adr << " " <<
        dec << setfill(' ') << setw(4) << dec << block_no << " " <<
        hex << setfill('0') << setw(4) << block_sz << " " <<
        hex << setfill(' ') << setw(8) << _BLOCK_ORDER(block_type) << " " <<
        "\n";
}

bool Utility::initTAPBlock(TargetMachine blockType, FileBlock& block)
{
    block.data.clear();
    block.tapeStartTime = -1;
    block.tapeEndTime = -1;
    block.leadToneCycles = -1;
    block.blockGap = -1;
    block.microToneCycles = -1;
    block.phaseShift = 180;
    block.trailerToneCycles = -1;
    block.blockType = blockType;

    return true;
}

bool Utility::encodeTAPHdr(FileBlock &block, string tapefileName, uint32_t fileLoadAdr, uint32_t loadAdr, uint32_t execAdr, uint32_t blockNo, uint32_t BlockSz)
{

    if (block.blockType <= BBC_MASTER) {
        for (int i = 0; i < sizeof(block.bbmHdr.name); i++) {
            if (i < tapefileName.length())
                block.bbmHdr.name[i] = tapefileName[i];
            else
                block.bbmHdr.name[i] = 0;
        }
        Utility::uint2bytes(fileLoadAdr, &block.bbmHdr.loadAdr[0], 4, true);
        Utility::uint2bytes(execAdr, &block.bbmHdr.execAdr[0], 4, true);
        Utility::uint2bytes(blockNo, &block.bbmHdr.blockNo[0], 2, true);
        Utility::uint2bytes(BlockSz, &block.bbmHdr.blockLen[0], 2, true);
        block.bbmHdr.blockFlag = 0x00; // b7 = last block, b6 = empty block, b0 = locked block
        block.bbmHdr.locked = 0;
    }
    else {
        for (int i = 0; i < sizeof(block.atomHdr.name); i++) {
            if (i < tapefileName.length())
                block.atomHdr.name[i] = tapefileName[i];
            else
                block.atomHdr.name[i] = 0;
        }
        block.atomHdr.loadAdrHigh = loadAdr / 256;
        block.atomHdr.loadAdrLow = loadAdr % 256;
        block.atomHdr.execAdrHigh = execAdr / 256;
        block.atomHdr.execAdrLow = execAdr % 256;
        block.atomHdr.lenHigh = BlockSz / 256;
        block.atomHdr.lenLow = BlockSz % 256;
    }

    return true;
}

void Utility::initbytes(Byte* bytes, Byte v, int n)
{
    for (int i = 0; i < n; i++) {
        bytes[i] = v;
    }
}

void Utility::initbytes(Bytes& bytes, Byte v, int n)
{
    for (int i = 0; i < n; i++) {
        bytes.push_back(v);
    }
}

void Utility::copybytes(Byte* from, Byte* to, int n)
{
    for (int i = 0; i < n; i++) {
        to[i] = from[i];
        to[i] = from[i];
    }
}

void Utility::copybytes(Byte * from, Bytes &to, int n)
{
    for (int i = 0; i < n; i++) {
        to.push_back(from[i]);
    }
}

uint32_t Utility::bytes2uint(Byte* bytes, int n, bool littleEndian)
{
    uint32_t u = 0;
    for (int i = 0; i < n && i < 4; i++) {
        if (littleEndian)
            u = (u << 8) & 0xffffff00 |  *(bytes + n - 1 - i);
        else
            u = (u << 8) & 0xffffff00 | + *(bytes+i);
    }
    return u;

}

void Utility::uint2bytes(uint32_t u, Byte* bytes, int n, bool littleEndian)
{
    for (int i = 0; i < n && i < 4; i++) {
        if (littleEndian)
            bytes[i] = (u >> (i*8)) & 0xff;
        else
            bytes[n - 1 - i] = (u >> (i*8)) & 0xff;
    }

}

bool Utility::readTapeFileName(TargetMachine block_type, BytesIter& data_iter, Bytes& data, Word& CRC, char* name)
{
    if (block_type == ACORN_ATOM)
        return Utility::readAtomTapeFileName(data_iter, data, CRC, name);
    else
        return Utility::readBBMTapeFileName(data_iter, data, CRC, name);
}

bool Utility::readAtomTapeFileName(BytesIter& data_iter, Bytes& data, Word& CRC, char *name)
{
    int i = 0;

    for (i = 0; i < ATM_HDR_NAM_SZ && *data_iter != 0xd && data_iter < data.end(); data_iter++) {
        name[i] = *data_iter;
        Utility::updateAtomCRC(CRC, *data_iter);
        i++;
    }
    for (int j = i; j < ATM_HDR_NAM_SZ; name[j++] = '\0');

    if (*data_iter != 0xd)
        return false;

    Utility::updateAtomCRC(CRC, *data_iter++); // should be 0xd;

    return true;
}

bool Utility::readBBMTapeFileName(BytesIter& data_iter, Bytes& data, Word& CRC, char *name)
{
    int i = 0;

    for (i = 0; i < BTM_HDR_NAM_SZ && *data_iter != 0x0 && data_iter < data.end(); data_iter++) {
        name[i] = *data_iter;
        Utility::updateBBMCRC(CRC, *data_iter);
        i++;
    }
    for (int j = i; j < BTM_HDR_NAM_SZ; name[j++] = '\0');

    if (*data_iter != 0x0)
        return false;

    Utility::updateBBMCRC(CRC, *data_iter++); // should be 0x0;

    return true;
}

void Utility::updateCRC(TargetMachine block_type, Word& CRC, Byte data)
{
    if (block_type == ACORN_ATOM)
        Utility::updateAtomCRC(CRC, data);
    else
        Utility::updateBBMCRC(CRC, data);
}

void Utility::updateAtomCRC(Word& CRC, Byte data)
{
    CRC = (CRC + data) % 256;
}

/*
 * Calculate 16-bit CRC for BBC Micro
 * 
 * The CRC is XMODEM with the polynom x^16 + x^12 + x^5 + 1 (0x11021) 
 * start value 0 and check value 0x31C3
 * 
 */
void Utility::updateBBMCRC(Word& CRC, Byte data)
{
    CRC ^=  data << 8;
    for (int i = 0; i < 8; i++)
    {
        if (CRC & 0x8000)
            CRC = (CRC << 1) ^ 0x11021;
        else
            CRC = CRC << 1;
    } 
}



string Utility::crDefaultOutFileName(string filePath, string fileExt)
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
string Utility::crDefaultOutFileName(string filePath)
{
    filesystem::path file_path = filePath;
    filesystem::path dir_path = file_path.parent_path();
    filesystem::path file_base = file_path.stem();

    string file_ext = file_path.extension().string();

    filesystem::path out_file_name = file_base.string() + "_out" + file_ext;

    filesystem::path file_out_path = dir_path / out_file_name;

    return file_out_path.string();
}
string Utility::crEncodedFileNamefromDir(string dirPath, TapeFile tapeFile, string fileExt)
{
    filesystem::path dir_path = dirPath;
    filesystem::path file_base = tapeFile.validFileName;

    string suffix;
    string file_ext;
    if (fileExt.size() == 0)
        file_ext = "";
    else
        file_ext = "." + fileExt;

    if (tapeFile.complete)
        suffix = file_ext;
    else
        suffix = "_incomplete_" + to_string(tapeFile.firstBlock) + "_" + to_string(tapeFile.lastBlock) + file_ext;

    filesystem::path output_file_name = dir_path / file_base;
    output_file_name += suffix;

    return output_file_name.string();;
}


/*
* Parse time on format mm:ss.ss
*/
double Utility::decodeTime(string time)
{
    stringstream ts(time);
    double hour, min, sec;
    char c;

    ts >> hour >> c >> min >> c >> sec;

    return (hour * 3600 + min * 60 + sec);

}

string Utility::encodeTime(double t) {
    char t_str[64];
    int t_h = (int)trunc(t / 3600);
    int t_m = (int)trunc((t - t_h * 3600) / 60);
    double t_s = t - t_h * 3600 - t_m * 60;
    sprintf(t_str, "%dh:%dm:%.6fs (%fs)", t_h, t_m, t_s, t);
    return string(t_str);
}

char Utility::digitToHex(int d)
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

string Utility::blockNameFromFilename(TargetMachine block_type, string filename)
{
    if (block_type == ACORN_ATOM)
        return Utility::atomBlockNameFromFilename(filename);
    else
        return Utility::bbmBlockNameFromFilename(filename);
}

string Utility::atomBlockNameFromFilename(string fn)
{
    string s = "";
    int len = (int)fn.length();
    int p = 0;
    int tape_file_pos = 0;
    while (p < len && tape_file_pos < ATOM_TAPE_NAME_LEN) {
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
        tape_file_pos++;
        p++;
        
    }

    return s;
}

/*
* Create a valid BBC Micro Tape Block Name from a DOS/Linux/MacOs filename.
*
* For BBC Micro a filename must start with a letter, and cannot contain spaces
* or punctuation marks. It cannot be longer than 10 characters in total.
* File names on cassettes can be up to ten characters long and can include any
* character except space ( and punctuation marks for compatibilty with disc
* file names).
*
*
* A filename longer than 10 characters will be truncated to 10 characters.
*
*/
string Utility::bbmBlockNameFromFilename(string fn)
{
    string s = "";
    int len = (int)fn.length();
    int p = 0;
    int tape_file_pos = 0;
    while (p < len && tape_file_pos < BBM_TAPE_NAME_LEN) {
        char c = fn[p];
        if (c == ' ' || c == '.')
            c = '_';
        s += c;
        tape_file_pos++;
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
string Utility::filenameFromBlockName(string fileName)
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
            s += Utility::digitToHex((int)c / 16);
            s += Utility::digitToHex((int)c % 16);
        }
    }

    return s;
}

void Utility::logData(int address, Byte* data, int sz)
{
    vector<Byte> bytes;
    for (int i = 0; i < sz; i++)
        bytes.push_back(*(data+i));
    BytesIter bi = bytes.begin();

    Utility::logData(address, bi, sz);

}

void Utility::logData(int address, BytesIter &data_iter, int data_sz) {

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

BlockType Utility::parseAtomTapeBlockFlag(AtomTapeBlockHdr hdr, int& blockLen) {

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

bool Utility::decodeFileBlockHdr(
    TargetMachine block_type,
    FileBlock block, int nReadChars,
    int& loadAdr, int& execAdr, int& blockSz,
    bool& isBasicProgram, string& fileName
)
{
    if (block_type == ACORN_ATOM)
        return Utility::decodeATMBlockHdr(block, nReadChars, loadAdr, execAdr, blockSz, isBasicProgram, fileName);
    else
        return Utility::decodeBTMBlockHdr(block, nReadChars, loadAdr, execAdr, blockSz, isBasicProgram, fileName);
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
// The read block is stored in a Wouter Ras ATM header format structure (FileBlock).
// This structure is similar to the tape structure but neither includes the block no
// nor the flags. This is most likely because the block no (and related flag bits)
// follows from the block sequence itself as blocks are numbered 0, 1, 2, ..., n-1
// where n = no of blocks..
//
bool Utility::decodeATMBlockHdr(
    FileBlock block, int nReadChars,
    int& loadAdr, int& execAdr, int& blockSz,
    bool& isBasicProgram, string& fileName
)
{
    loadAdr = -1;
    execAdr = -1;
    blockSz = -1;
    isBasicProgram = false;
    fileName = "???";

    // Get TAP filename if it was completey read
    if (nReadChars > 4) { // name comes after 4-bytes preamble
        fileName = "";
        int sz = 0;
        for (sz = 0; sz < sizeof(block.atomHdr.name) && block.atomHdr.name[sz] != 0x0; sz++) {
            if (block.atomHdr.name[sz] < 0x20 || block.atomHdr.name[sz] > 0x7f) {
                fileName += "?";
                break;
            }
            else
                fileName += block.atomHdr.name[sz];
        }
    } else
        return false; // If filename not read then the rest will not have been read either...

    // Calculate offset to the rest of the block's parameters (that comes after the preamble and filename)
    int offset = 4 + fileName.length();

    if (nReadChars > offset + (long)  & ((AtomTapeBlockHdr*)0)->loadAdrLow)
        loadAdr = block.atomHdr.loadAdrHigh * 256 + block.atomHdr.loadAdrLow;

    if (nReadChars > offset + (long)&((AtomTapeBlockHdr*)0)->dataLenM1)
        blockSz = block.atomHdr.lenHigh * 256 + block.atomHdr.lenLow;

    if (nReadChars > offset + (long) &((AtomTapeBlockHdr*)0) -> execAdrLow)
        execAdr = block.atomHdr.execAdrHigh * 256 + block.atomHdr.execAdrLow;

    if (execAdr == 0xc2b2)
        isBasicProgram = true;

    return (nReadChars >= sizeof(AtomTapeBlockHdr) + offset);
}

bool Utility::decodeBTMBlockHdr(
    FileBlock block, int nReadChars,
    int& loadAdr, int& execAdr, int& blockSz,
    bool& isBasicProgram, string& fileName
) {
    loadAdr = Utility::bytes2uint(&block.bbmHdr.loadAdr[0], 4, true);
    execAdr = Utility::bytes2uint(&block.bbmHdr.execAdr[0], 4, true);
    blockSz = Utility::bytes2uint(&block.bbmHdr.blockLen[0], 2, true);
    isBasicProgram = false;
    fileName = block.bbmHdr.name;

    return true;
}

bool Utility::decodeBBMTapeHdr(BBMTapeBlockHdr& hdr, FileBlock& block,
    uint32_t&loadAdr, uint32_t&execAdr, uint32_t &blockLen, int &blockNo, uint32_t &nextAdr, bool &isLastBlock
)
{
    // Extract header information and update BBM block with it
    loadAdr = Utility::bytes2uint(&hdr.loadAdr[0], 4, true);
    execAdr = Utility::bytes2uint(&hdr.execAdr[0], 4, true);
    blockLen = Utility::bytes2uint(&hdr.blockLen[0], 2, true);
    blockNo = Utility::bytes2uint(&hdr.blockNo[0], 2, true);
    nextAdr = Utility::bytes2uint(&hdr.nextFileAdr[0], 4, true);
    isLastBlock = (hdr.blockFlag & 0x80) != 0; // b7 = last block
    Utility::copybytes(&hdr.loadAdr[0], &block.bbmHdr.loadAdr[0], 4);
    Utility::copybytes(&hdr.execAdr[0], &block.bbmHdr.execAdr[0], 4);
    Utility::copybytes(&hdr.blockNo[0], &block.bbmHdr.blockNo[0], 2);
    Utility::copybytes(&hdr.blockLen[0], &block.bbmHdr.blockLen[0], 2);
    block.bbmHdr.locked = (hdr.blockFlag & 0x1) != 0; // b0 = locked block
    block.bbmHdr.blockFlag = hdr.blockFlag;

    return true;
}

bool Utility::decodeAtomTapeHdr(AtomTapeBlockHdr& hdr, FileBlock& block,
    int &loadAdr, int &execAdr, int &blockLen, int& blockNo, BlockType &blockType
)
{
    loadAdr = hdr.loadAdrHigh * 256 + hdr.loadAdrLow;
    execAdr = hdr.execAdrHigh * 256 + hdr.execAdrLow;
    blockType = Utility::parseAtomTapeBlockFlag(hdr, blockLen);
    blockNo = hdr.blockNoHigh * 256 + hdr.blockNoLow;

    block.atomHdr.execAdrHigh = hdr.execAdrHigh;
    block.atomHdr.execAdrLow = hdr.execAdrLow;
    block.atomHdr.lenHigh = blockLen / 256;
    block.atomHdr.lenLow = blockLen % 256;
    block.atomHdr.loadAdrHigh = hdr.loadAdrHigh;
    block.atomHdr.loadAdrLow = hdr.loadAdrLow;

    return true;
}

bool Utility::encodeTapeHdr(TargetMachine block_type, FileBlock& block, int block_no, int n_blocks, Bytes& hdr)
{
    block.blockType = block_type;
    if (block.blockType == ACORN_ATOM)
        return Utility::encodeAtomTapeHdr(block, block_no, n_blocks, hdr);
    else
        return Utility::encodeBBMTapeHdr(block, hdr);
}

//
// <name:1-10> <load adr:4> <exec adr:4> <block no:2> <block len:2> <block flag:1> <next adr:4>
//
bool Utility::encodeBBMTapeHdr(FileBlock& block, Bytes& hdr)
{
    // store block name
    int name_len = 0;
    for (; name_len < sizeof(block.bbmHdr.name) && block.bbmHdr.name[name_len] != 0; name_len++);
    for (int i = 0; i < name_len; i++)
        hdr.push_back(block.bbmHdr.name[i]);
    hdr.push_back(0x0);

    Utility::copybytes(block.bbmHdr.loadAdr, hdr, 4); // load address
    Utility::copybytes(block.bbmHdr.execAdr, hdr, 4); // exec address
    Utility::copybytes(block.bbmHdr.blockNo, hdr, 2); // block no
    Utility::copybytes(block.bbmHdr.blockLen, hdr, 2); // block len
    hdr.push_back(block.bbmHdr.blockFlag); // block flag
    Utility::initbytes(hdr, 0, 4); // next address

    return true;
}

//
// header = <name:2 - 14> <flag:1> <block no:2> <block len -1:1> <exec adr:2> <load adr:2>
//
bool Utility::encodeAtomTapeHdr(FileBlock& block, int block_no, int n_blocks, Bytes &hdr)
{
    int data_len = block.atomHdr.lenHigh * 256 + block.atomHdr.lenLow;  // get data length

    Byte b7 = (block_no < n_blocks - 1 ? 0x80 : 0x00);          // calculate flags
    Byte b6 = (data_len > 0 ? 0x40 : 0x00);
    Byte b5 = (block_no != 0 ? 0x20 : 0x00);

    // store block name
    int name_len = 0;
    for (; name_len < sizeof(block.atomHdr.name) && block.atomHdr.name[name_len] != 0; name_len++);
    for (int i = 0; i < name_len; i++)
        hdr.push_back(block.atomHdr.name[i]);
    hdr.push_back(0xd);

    hdr.push_back(b7 | b6 | b5);                        // store flags

    hdr.push_back((block_no >> 8) & 0xff);              // store block no
    hdr.push_back(block_no & 0xff);

    hdr.push_back((data_len > 0 ? data_len - 1 : 0));   // store length - 1

    hdr.push_back(block.atomHdr.execAdrHigh);     // store execution address
    hdr.push_back(block.atomHdr.execAdrLow);

    hdr.push_back(block.atomHdr.loadAdrHigh);     // store load address
    hdr.push_back(block.atomHdr.loadAdrLow);

    return true;
}

bool Utility::initTapeHdr(FileBlock& block, CapturedBlockTiming &block_info)
{
    block.blockGap = block_info.block_gap;
    block.preludeToneCycles = block_info.prelude_lead_tone_cycles;
    block.leadToneCycles = block_info.lead_tone_cycles;
    block.microToneCycles = block_info.micro_tone_cycles;
    block.trailerToneCycles = block_info.trailer_tone_cycles;
    block.phaseShift = block_info.phase_shift;
    block.tapeStartTime = block_info.start;
    block.tapeEndTime = block_info.end;

    return true;
}

bool Utility::initTapeHdr(FileBlock& block)
{
    block.blockGap = -1;
    block.preludeToneCycles = -1;
    block.leadToneCycles = -1;
    block.microToneCycles = -1;
    block.trailerToneCycles = -1;
    block.phaseShift = 180;
    block.tapeStartTime = -1;
    block.tapeEndTime = -1;

    if (block.blockType <= BBC_MASTER) {
        initbytes(&block.bbmHdr.loadAdr[0], 0xff, 4);
        initbytes(&block.bbmHdr.execAdr[0], 0xff, 4);
        initbytes(&block.bbmHdr.blockNo[0], 0xff, 2);
        initbytes(&block.bbmHdr.blockLen[0], 0xff, 2);
        block.bbmHdr.locked = false;
        block.bbmHdr.name[0] = 0xff;
        block.bbmHdr.blockFlag = 0x00;
    }
    else if (block.blockType == ACORN_ATOM) {
        block.atomHdr.execAdrHigh = 0xff;
        block.atomHdr.execAdrLow = 0xff;
        block.atomHdr.lenHigh = 0xff;
        block.atomHdr.lenLow = 0xff;
        block.atomHdr.loadAdrHigh = 0xff;
        block.atomHdr.loadAdrLow = 0xff;
        block.atomHdr.name[0] = 0xff;
    }

    return true;
}