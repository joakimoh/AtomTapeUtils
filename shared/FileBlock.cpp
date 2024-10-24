#include "FileBlock.h"
#include "Utility.h"
#include <iostream>
#include <sstream>
#include <cstdint>
#include "Logging.h"
#include <filesystem>
#include "WaveSampleTypes.h"

BlockType FileBlock::getBlockType()
{
    return blockType;
}

void FileBlock::setBlockType(BlockType bt)
{
    blockType = bt;
}

//
// Iterate over all blocks and update their block types (SINGLE, FIRST, OTHER, LAST)
//
// Required when the no of blocks in the tape file is not known
// when creating the tape file (but only after creation of it)
//
bool TapeFile::setBlockTypes()
{
    if (this->blocks.size() == 0)
        return true;

    // Type of last block
    this->blocks[this->blocks.size() - 1].setBlockType(BlockType::Last);
    if (this->metaData.targetMachine <= BBC_MASTER)
        this->blocks[this->blocks.size() - 1].bbmHdr.blockFlag |= 0x80;

    // Type of first block
    if (this->blocks.size() == 1)
        this->blocks[0].setBlockType(BlockType::Single); // if first block = last block = only block
    else
        this->blocks[0].setBlockType(BlockType::First);

    // Types of intermediate blocks
    for (int i = 1; i < this->blocks.size()-1; i++)
        this->blocks[i].setBlockType(BlockType::Other);

    return true;
}

void TapeFile::init()
{
    init(TargetMachine::UNKNOWN_TARGET);
}

void TapeFile::init(TargetMachine targetMachine)
{
    metaData.init(targetMachine);
    blocks.clear();
    complete = false;
    corrupted = false;
    firstBlock = -1;
    lastBlock = -1;
    programName = "FAILED";
    validTiming = false;
    baudRate = 300;
}

FileBlock::FileBlock(TargetMachine bt) {
    targetMachine = bt;
}

string FileBlock::tapeField(int n)
{
    if (targetMachine <= BBC_MASTER)
        return bbmTapeBlockHdrFieldName(n);
    else
        return atomTapeBlockHdrFieldName(n);
}

string FileBlock::atomTapeBlockHdrFieldName(int offset)
{
    switch (offset) {
    case 0: return "flags";
    case 1: return "blockNoHigh";
    case 2: return "blockNoLow";
    case 3: return "dataLenM1";
    case 4: return "execAdrHigh";
    case 5: return "execAdrLow";
    case 6: return "loadAdrHigh";
    case 7: return "loadAdrLow";
    default: return "???";
    }

}

string FileBlock::bbmTapeBlockHdrFieldName(int offset)
{
    switch (offset) {
    case 0: return "loadAdrByte0";
    case 1: return "loadAdrByte1";
    case 2: return "loadAdrByte2";
    case 3: return "loadAdrByte3";
    case 4: return "execAdrByte0";
    case 5: return "execAdrByte1";
    case 6: return "execAdrByte2";
    case 7: return "execAdrByte3";
    case 8: return "blockNoLSB";
    case 9: return "blockNoMSB";
    case 10: return "blockLenLSB";
    case 11: return "blockLenMSB";
    case 12: return "blockFlag";
    case 13: return "nextLoadAdrByte0";
    case 14: return "nextLoadAdrByte1";
    case 15: return "nextLoadAdrByte2";
    case 16: return "nextLoadAdrByte3";
    case 17: return "calc_hdr_CRC";
    default: return "???";
    }

}

string FileBlock::blockName() {
    if (targetMachine == ACORN_ATOM)
        return atomHdr.name;
    else
        return bbmHdr.name;
}


bool FileBlock::init(CapturedBlockTiming& block_info)
{
    init();

    this->preludeToneCycles = block_info.prelude_lead_tone_cycles;
    this->leadToneCycles = block_info.lead_tone_cycles;
    this->microToneCycles = block_info.micro_tone_cycles;
    this->trailerToneCycles = block_info.trailer_tone_cycles;
    this->blockGap = block_info.block_gap;

    this->phaseShift = block_info.phase_shift;
    this->tapeStartTime = block_info.start;
    this->tapeEndTime = block_info.end;

    return true;
}

bool FileBlock::init()
{
    this->data.clear();
    
    this->blockNo = -1;
    this->blockType= BlockType::Unknown;

    this->preludeToneCycles = -1; 
    this->leadToneCycles = -1;
    this->microToneCycles = -1;
    this->trailerToneCycles = -1;
    this->blockGap = -1;
    this->phaseShift = -1;
    
    this->tapeEndTime = -1;
    this->tapeStartTime = -1;
    this->trailerToneCycles = -1;

    if (this->targetMachine <= BBC_MASTER)
        Utility::initbytes((Byte *)  & this->bbmHdr.name[0], 0, sizeof(BTMHdr));
    else
        Utility::initbytes((Byte*)&this->atomHdr.name[0], 0, sizeof(ATMHdr));

    return true;
}

bool FileBlock::updateCRC(Word &crc, Byte data)
{
    return updateCRC(targetMachine, crc, data);

}

bool FileBlock::updateCRC(TargetMachine targetMachine, Word& crc, Byte data)
{
    if (targetMachine <= BBC_MASTER)
        return updateBBMCRC(crc, data);
    else if (targetMachine == ACORN_ATOM)
        return updateAtomCRC(crc, data);
    else
        return false;
}


bool FileBlock::updateAtomCRC(Word& CRC, Byte data)
{
    CRC = (CRC + data) % 256;
    return true;
}

/*
 * Calculate 16-bit CRC for BBC Micro
 *
 * The CRC is XMODEM with the polynom x^16 + x^12 + x^5 + 1 (0x11021)
 * start value 0 and check value 0x31C3
 *
 */
bool FileBlock::updateBBMCRC(Word& CRC, Byte data)
{
    CRC ^= data << 8;
    for (int i = 0; i < 8; i++)
    {
        if (CRC & 0x8000)
            CRC = (CRC << 1) ^ 0x11021;
        else
            CRC = CRC << 1;
    }
     return true;
}

int FileBlock::tapeHdrSz()
{
    if (targetMachine == ACORN_ATOM)
        return sizeof(AtomTapeBlockHdr);
    else if (targetMachine <= BBC_MASTER)
        return sizeof(BBMTapeBlockHdr);
    else
        return -1;
}

int FileBlock::dataSz()
{
    if (targetMachine == ACORN_ATOM)
        return atomHdr.lenHigh * 256 + atomHdr.lenLow;
    else if (targetMachine <= BBC_MASTER)
        return Utility::bytes2uint(&bbmHdr.blockLen[0], 2, true);
    else
        return -1;
}

uint32_t FileBlock::loadAdr()
{
    if (targetMachine <= BBC_MASTER)
        return Utility::bytes2uint(&bbmHdr.loadAdr[0], 4, true);
    else
        return atomHdr.loadAdrHigh * 256 + atomHdr.loadAdrLow;
}

uint32_t FileBlock::execAdr()
{
    if (targetMachine <= BBC_MASTER)
        return Utility::bytes2uint(&bbmHdr.execAdr[0], 4, true);
    else
        return atomHdr.execAdrHigh * 256 + atomHdr.execAdrLow;
}

bool FileBlock::lastBlock()
{
    return blockType == BlockType::Last || blockType == BlockType::Single;
}

bool FileBlock::firstBlock()
{
    return blockType == BlockType::First || blockType == BlockType::Single;
}

bool FileBlock::decodeTapeHdr(Bytes &name, Bytes &hdr_bytes)
{
    if (targetMachine == ACORN_ATOM) {

        // Add block name to header (terminating 0xd excluded)
        for (int i = 0; i < name.size()-1 && i < ATM_HDR_NAM_SZ; i++)
            atomHdr.name[i] = name[i];
        for (int i = (int) name.size()-1; i < ATM_HDR_NAM_SZ; i++)
            atomHdr.name[i] = '\0';

    }
    else if (targetMachine <= BBC_MASTER) {

        // Add block name to header
        for (int i = 0; i < name.size() && i < BTM_HDR_NAM_SZ; i++)
            bbmHdr.name[i] = name[i];
        for (int i = (int) name.size(); i < BTM_HDR_NAM_SZ; i++)
            bbmHdr.name[i] = '\0';    
    }
    else
        return false;

    // Decode rest of header
    return decodeTapeHdr(hdr_bytes);
}

bool FileBlock::decodeTapeHdr(Bytes& hdr_bytes)
{
    if (targetMachine == ACORN_ATOM) {

        // Check that the no of remaining bytes are sufficient to hold the header
        if (hdr_bytes.size() < sizeof(AtomTapeBlockHdr))
            return false;

        // Read rest of header into the ATM Block header
        atomHdr.loadAdrLow = hdr_bytes[ATOM_TAPE_BLOCK_LOAD_ADR_L];
        atomHdr.loadAdrHigh = hdr_bytes[ATOM_TAPE_BLOCK_LOAD_ADR_H];
        atomHdr.execAdrLow = hdr_bytes[ATOM_TAPE_BLOCK_EXEC_ADR_L];
        atomHdr.execAdrHigh = hdr_bytes[ATOM_TAPE_BLOCK_EXEC_ADR_H];
        Byte block_sz_M1 = hdr_bytes[ATOM_TAPE_BLOCK_LEN];
        Byte flags = hdr_bytes[ATOM_TAPE_BLOCK_FLAGS]; // b7: not  last block.; b6: block has data; b5:not 1st block.
        int block_sz;
        if ((flags & 0x40) == 0x40)
            block_sz = block_sz_M1 + 1;
        else
            block_sz = 0;
        atomHdr.lenLow = block_sz % 256;
        atomHdr.lenHigh = block_sz / 256;

        // Extract block no
        int block_no_H = hdr_bytes[ATOM_TAPE_BLOCK_NO_H];
        int block_no_L = hdr_bytes[ATOM_TAPE_BLOCK_NO_L];
        blockNo = block_no_H * 256 + block_no_L;

        // Determine block type (First, Other or Last)
        if ((flags & 0x80) == 0x80) { // b7 = 1 => Not the last block (i.e., first or middle block)
            if ((flags & 0x20) == 0x20) // b5 = 1 => Not the first block (i.e. middle block)
                blockType = BlockType::Other;
            else
                blockType = BlockType::First;
        }
        else { // The last block (could also be the first block if there is only one block in total
            if ((flags & 0x20) == 0x20) // b5 = 1 => Not the first block
                blockType = BlockType::Last;
            else
                blockType = BlockType::Single; // Single block which is both the First and Last block
        }

    }
    else if (targetMachine <= BBC_MASTER) {

        // Check that the no of remaining bytes are sufficient to hold the header
        if (hdr_bytes.size() < sizeof(BBMTapeBlockHdr))
            return false;

        // Read rest of header into the BBM Block header
        for (int i = 0; i < 4; bbmHdr.loadAdr[i++] = hdr_bytes[BBM_TAPE_BLOCK_LOAD_ADR + i]);
        for (int i = 0; i < 4; bbmHdr.execAdr[i++] = hdr_bytes[BBM_TAPE_BLOCK_EXEC_ADR + i]);
        for (int i = 0; i < 2; bbmHdr.blockNo[i++] = hdr_bytes[BBM_TAPE_BLOCK_NO + i]);
        for (int i = 0; i < 2; bbmHdr.blockLen[i++] = hdr_bytes[BBM_TAPE_BLOCK_LEN + i]);
        blockNo = Utility::bytes2uint(&bbmHdr.blockNo[0], 2, true);

        // Decode flag - b7 = last block, b6 = empty block, b0 = locked block
        bbmHdr.blockFlag = hdr_bytes[BBM_TAPE_BLOCK_FLAGS];
        bbmHdr.locked = (bbmHdr.blockFlag & 0x1) != 0; // b0 = locke0d ;block
        if (bbmHdr.blockFlag & 0x80) {
            if (blockNo == 0)
                blockType = BlockType::Single;
            else
                blockType = BlockType::Last;
        }
        else if (blockNo == 0)
            blockType = BlockType::First;
        else
            blockType = BlockType::Other;

    }
    else
        return false;

    return true;
}

bool FileBlock::encodeTapeHdr(Bytes &hdr)
{
    if (targetMachine == ACORN_ATOM) {
        
        //header = <name:2 - 14> <flag:1> <block no:2> <block len -1:1> <exec adr:2> <load adr:2>

        int data_len = atomHdr.lenHigh * 256 + atomHdr.lenLow;  // get data length

        Byte b7 = (blockType != BlockType::Last && blockType != BlockType::Single ? 0x80 : 0x00); // Bit 7  set if not the last block 
        Byte b6 = (data_len > 0 ? 0x40 : 0x00); // Bit 6  set if  block contains data.
        Byte b5 = (blockType != BlockType::First && blockType != BlockType::Single? 0x20 : 0x00); // Bit 5 set if not the first block

        // Write block name
        int name_len = 0;
        for (; name_len < sizeof(atomHdr.name) && atomHdr.name[name_len] != 0; name_len++);
        for (int i = 0; i < name_len; i++) hdr.push_back(atomHdr.name[i]);
        hdr.push_back(0xd); // 0xd as terminator

        // Write rest of block
        hdr.push_back(b7 | b6 | b5);                        // store flags
        hdr.push_back((blockNo >> 8) & 0xff);              // store block no
        hdr.push_back(blockNo & 0xff);
        hdr.push_back((data_len > 0 ? data_len - 1 : 0));   // store length - 1
        hdr.push_back(atomHdr.execAdrHigh);     // store execution address
        hdr.push_back(atomHdr.execAdrLow);
        hdr.push_back(atomHdr.loadAdrHigh);     // store load address
        hdr.push_back(atomHdr.loadAdrLow);
    }

    else if (targetMachine <= BBC_MASTER) {

        // <name:1-10> <load adr:4> <exec adr:4> <block no:2> <block len:2> <block flag:1> <next adr:4>

        // store block name
        int name_len = 0;
        for (; name_len < sizeof(bbmHdr.name) && bbmHdr.name[name_len] != 0; name_len++);
        for (int i = 0; i < name_len; i++)
            hdr.push_back(bbmHdr.name[i]);
        hdr.push_back(0x0); // 0x0 as terminator

        Utility::copybytes(bbmHdr.loadAdr, hdr, 4); // load address
        Utility::copybytes(bbmHdr.execAdr, hdr, 4); // exec address
        Utility::copybytes(bbmHdr.blockNo, hdr, 2); // block no
        Utility::copybytes(bbmHdr.blockLen, hdr, 2); // block len
        hdr.push_back(bbmHdr.blockFlag); // block flag
        Utility::initbytes(hdr, 0, 4); // next address
    }
    else
        return false;

    return true;
    
}

bool FileBlock::logHdr(ostream* fout)
{
    if (fout == NULL)
        return true;

    if (targetMachine == ACORN_ATOM) {
        uint32_t exec_adr = atomHdr.execAdrHigh * 256 + atomHdr.execAdrLow;
        uint32_t load_adr = atomHdr.loadAdrHigh * 256 + atomHdr.loadAdrLow;
        uint32_t block_sz = atomHdr.lenHigh * 256 + atomHdr.lenLow;
        *fout << setw(13) << setfill(' ') << atomHdr.name << " " <<
            hex << setfill('0') << setw(4) << load_adr << " " <<
            hex << setfill('0') << setw(4) << load_adr + block_sz - 1 << " " <<
            hex << setfill('0') << setw(4) << exec_adr << " " <<
            hex << setfill('0') << setw(4) << blockNo << " " <<
            hex << setfill('0') << setw(4) << block_sz << " " << setfill(' ') << " " <<
            setw(8) << _BLOCK_ORDER(this->blockType);
        if (leadToneCycles != -1 || microToneCycles != -1 || blockGap != -1) {
            *fout << Utility::roundedPD(" LEAD TONE ", (double)leadToneCycles / F2_FREQ, ": ") <<
                Utility::roundedPD("MICRO TONE ", (double)microToneCycles / F2_FREQ, ": ") <<
                Utility::roundedPD("GAP ", blockGap, "");
        }
      
    }
    else {
        uint32_t exec_adr = Utility::bytes2uint(&bbmHdr.execAdr[0], 4, true);
        uint32_t file_load_adr = Utility::bytes2uint(&bbmHdr.loadAdr[0], 4, true);
        uint32_t block_sz = Utility::bytes2uint(&bbmHdr.blockLen[0], 2, true);
        uint32_t block_no = Utility::bytes2uint(&bbmHdr.blockNo[0], 2, true);
        Byte flag = bbmHdr.blockFlag;
        *fout << setw(10) << setfill(' ') << bbmHdr.name << " " <<
            hex << setfill('0') << setw(8) << file_load_adr + 256 * block_no << " " <<
            hex << setfill('0') << setw(8) << file_load_adr + 256 * block_no + block_sz - 1 << " " <<
            hex << setfill('0') << setw(8) << exec_adr << " " <<
            hex << setfill('0') << setw(4) << block_no << " " <<
            hex << setfill('0') << setw(4) << block_sz << " " <<
            hex << setfill('0') << setw(2) << hex << (int)flag << setfill(' ') << " " <<
            setw(8) << _BLOCK_ORDER(this->blockType);
        if (leadToneCycles != -1 || trailerToneCycles != -1 || blockGap != -1) {
            *fout <<
                " PRELUDE LEAD " << setw(5) << (preludeToneCycles > 0 ? to_string(preludeToneCycles) : "-") << " cycles: " <<
                Utility::roundedPD(" (POSTLUDE) LEAD ", (double)leadToneCycles / F2_FREQ, ": ") <<
                Utility::roundedPD(" TRAILER ", (double)trailerToneCycles / F2_FREQ, ": ") <<
                Utility::roundedPD("GAP ", blockGap, "");
        }
    }

    if (this->tapeStartTime < 0)
        *fout << "\n";
    else
        *fout << " " << Utility::encodeTime(this->tapeStartTime) << " => " << Utility::encodeTime(this->tapeEndTime) << "\n";

    return true;
}

bool FileBlock::logHdr()
{
    return logHdr(&cout);
}

void TapeFile::logTAPFileHdr()
{
    logTAPFileHdr(&cout);
}

void TapeFile::logTAPFileHdr(ostream* fout)
{
    if (fout == NULL || blocks.size() == 0)
        return;

    FileBlock& block = blocks[0];
    FileBlock& last_block = blocks[blocks.size()-1];

    if (metaData.targetMachine == ACORN_ATOM) {
        uint32_t exec_adr = block.atomHdr.execAdrHigh * 256 + block.atomHdr.execAdrLow;
        uint32_t load_adr = block.atomHdr.loadAdrHigh * 256 + block.atomHdr.loadAdrLow;
        uint32_t n_blocks = (uint32_t)blocks.size();
        uint32_t file_sz = 0;
        for (uint32_t i = 0; i < n_blocks; i++)
            file_sz += blocks[i].atomHdr.lenHigh * 256 + blocks[i].atomHdr.lenLow;
        *fout << setw(13) << block.atomHdr.name << " " <<
            hex << setfill('0') << setw(4) << load_adr << " " <<
            hex << setfill('0') << setw(4) << load_adr + file_sz - 1 << " " <<
            hex << setfill('0') << setw(4) << exec_adr << " " <<
            dec << setfill(' ') << setw(5) << n_blocks;
    }
    else if (metaData.targetMachine <= BBC_MASTER) {
        uint32_t exec_adr = Utility::bytes2uint(&block.bbmHdr.execAdr[0], 4, true);
        uint32_t load_adr = Utility::bytes2uint(&block.bbmHdr.loadAdr[0], 4, true);
        uint32_t block_no = Utility::bytes2uint(&block.bbmHdr.blockNo[0], 2, true);
        uint32_t file_sz = 0;
        uint32_t n_blocks = (uint32_t)blocks.size();
        for (uint32_t i = 0; i < n_blocks; i++)
            file_sz += Utility::bytes2uint(&blocks[i].bbmHdr.blockLen[0], 2, true);
        *fout << setw(10) << block.bbmHdr.name << " " <<
            hex << setfill('0') << setw(8) << load_adr << " " <<
            hex << setfill('0') << setw(8) << load_adr + file_sz - 1 << " " <<
            hex << setfill('0') << setw(8) << exec_adr << " " <<
            dec << setfill(' ') << setw(5) << n_blocks << " " <<
            hex << setfill('0') << setw(4) << file_sz;
    }
    else {
    }

    if (block.tapeStartTime < 0)
        *fout << setfill(' ') << "\n";
    else
        *fout << setfill(' ') << " " << Utility::encodeTime(block.tapeStartTime) << " => " << Utility::encodeTime(last_block.tapeEndTime) << "\n";

}

bool FileBlock::encodeTAPHdr(
    string tapefileName, uint32_t fileLoadAdr, uint32_t loadAdr, uint32_t execAdr, uint32_t blockNo, uint32_t BlockSz
)
{
    if (targetMachine <= BBC_MASTER) {
        for (int i = 0; i < sizeof(bbmHdr.name); i++) {
            if (i < tapefileName.length())
                bbmHdr.name[i] = tapefileName[i];
            else
                bbmHdr.name[i] = 0;
        }
        Utility::uint2bytes(fileLoadAdr, &bbmHdr.loadAdr[0], 4, true);
        Utility::uint2bytes(execAdr, &bbmHdr.execAdr[0], 4, true);
        Utility::uint2bytes(blockNo, &bbmHdr.blockNo[0], 2, true);
        Utility::uint2bytes(BlockSz, &bbmHdr.blockLen[0], 2, true);
        bbmHdr.blockFlag = 0x00; // b7 = last block, b6 = empty block, b0 = locked block
        bbmHdr.locked = 0;
    }
    else {
        for (int i = 0; i < sizeof(atomHdr.name); i++) {
            if (i < tapefileName.length())
                atomHdr.name[i] = tapefileName[i];
            else
                atomHdr.name[i] = 0;
        }
        atomHdr.loadAdrHigh = loadAdr / 256;
        atomHdr.loadAdrLow = loadAdr % 256;
        atomHdr.execAdrHigh = execAdr / 256;
        atomHdr.execAdrLow = execAdr % 256;
        atomHdr.lenHigh = BlockSz / 256;
        atomHdr.lenLow = BlockSz % 256;

    }
    if (blockNo == 0)
        this->blockType = BlockType::First;
    else
        this->blockType = BlockType::Other;
    this->blockNo = blockNo;

    return true;
}

bool FileBlock::readTapeFileName(TargetMachine target_machine, BytesIter& data_iter, Bytes& data, Word& CRC)
{
    if (target_machine == ACORN_ATOM) {
        int i = 0;


        for (i = 0; i < ATM_HDR_NAM_SZ && *data_iter != 0xd && data_iter < data.end(); data_iter++) {
            atomHdr.name[i] = *data_iter;
            updateAtomCRC(CRC, *data_iter);
            i++;
        }
        for (int j = i; j < ATM_HDR_NAM_SZ; atomHdr.name[j++] = '\0');

        if (*data_iter != 0xd)
            return false;

        updateAtomCRC(CRC, *data_iter++);  // should be 0xd;

    }
    else {
        int i = 0;

        for (i = 0; i < BTM_HDR_NAM_SZ && *data_iter != 0x0 && data_iter < data.end(); data_iter++) {
           
            bbmHdr.name[i] = *data_iter;
            updateBBMCRC(CRC, *data_iter);
            i++;
        }
        for (int j = i; j < BTM_HDR_NAM_SZ; bbmHdr.name[j++] = '\0');

        if (*data_iter != 0x0)
            return false;
    }

    return true;
}


/*
* Create a valid DOS/Linux/MacOs filename from the disc or tape program name.
*
* Characters that are not allowed in a DOS/Linux/MacOs filename are:
* /<>:"\|?* and ASCII 0-31
*
* '_' is used as an escape sequence (_hh where hh is the ASCII in hex)
* and will therefore not either be allowed in the filename chosen.
*
* Filenames cannot either end in SPACE or DOT.
*/
string TapeFile::crValidHostFileName(string programName)
{
    string invalid_chars = "\"/<>:|?*";
    string s = "";
    for (auto& c : programName) {
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

string TapeFile::crValidDiscFilename(string programName)
{
    return crValidDiscFilename(metaData.targetMachine, programName);
}

//
// Create a valid Acorn DFS filename from a host or tape file name.
//
// The filename needs to be truncated to 7 characters.
// Valid characters are the printable ASCII characters between &20 and &7E inclusive, except . : " # * and space
//
string TapeFile::crValidDiscFilename(TargetMachine targetMachine, string programName)
{
    string invalid_chars = ".:\"/#*";
    string s = "";
    for (auto& c : programName) {
        if (s.length() < 7 && c >= (char) 0x20 && c <= (char) 0x7e && invalid_chars.find(c) == string::npos)
            s += c;
    }

    return s;
}

string TapeFile::crValidBlockName(string fileName)
{
    return crValidBlockName(metaData.targetMachine, fileName);
}

string TapeFile::crValidBlockName(TargetMachine targetMachine, string fileName)
{
    if (targetMachine == ACORN_ATOM)
        return crValidBlockName("", ATOM_TAPE_NAME_LEN, fileName);
    else
        return crValidBlockName(" ", BBM_TAPE_NAME_LEN, fileName);
}

//
// Create a valid Tape Block Name from an Acorn DFS or DOS/Linux/MacOs filename.
//
// Filenames can be be up to nameLen chars and can include any letter except
// the ones in invalidChars.
// 
// Sequences of '__' & '_'<hh> in the filename will be replaced by '_' and (char) 0x<hh>
//
//
string TapeFile::crValidBlockName(string invalidChars, int nameLen, string fn)
{

    string s = "";
    int len = (int)fn.length();
    int p = 0;
    int tape_file_pos = 0;
    while (p < len && tape_file_pos < nameLen) {
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
                c = (char)h;
                p += 2;
            }
            else { // ERROR
                return "";
            }
        }
        if (invalidChars.find(c) == string::npos)
            s += c;
        tape_file_pos++;
        p++;

    }

    return s;
}



int TapeFile::size()
{
    int sz = 0;
    for (int i = 0; i < this->blocks.size(); i++)
        sz += (int) this->blocks[i].data.size();

    return sz;
}