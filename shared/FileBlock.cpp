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
    header.init(targetMachine);
    blocks.clear();
    complete = false;
    corrupted = false;
    firstBlock = -1;
    lastBlock = -1;
    header.name = "FAILED";
    validTiming = false;
    baudRate = 300;
}

FileBlock::FileBlock(TargetMachine t) {
    targetMachine = t;
}

FileBlock::FileBlock(TargetMachine t, string blockName, uint32_t blockNo, uint32_t LA, uint32_t EA, uint32_t sz)
{
    this->targetMachine = t;
    this->name = blockName;
    this->no = blockNo;
    this->loadAdr = LA;
    this->execAdr = EA;
    this->size = sz;
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
    
    this->no = -1;
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

    this->loadAdr = 0x0;
    this->execAdr = 0x0;
    this->size = 0x0;
    this->no = 0x0;
    this->locked = false;
    this->nextAdr = 0x0;

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

int FileBlock::tapeHdrSz()
{
    if (targetMachine <= BBC_MASTER)
        return BBM_TAPE_HDR_SZ;
    else
        return ATOM_TAPE_HDR_SZ;
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

bool FileBlock::decodeTapeBlockHdr(Bytes &name_bytes, Bytes &hdr_bytes, bool limitBlockNo)
{
    // Decode name
    name = "";
    Byte terminator = 0xd;
    if (targetMachine <= BBC_MASTER)
        terminator = 0x0;
    for (int i = 0; i < name_bytes.size() && name_bytes[i] != terminator; i++) {
        name = name + (char)name_bytes[i];
    }
    
    // Decode rest of header
    if (targetMachine <= BBC_MASTER) {

        // Check that the no of remaining bytes are sufficient to hold the header
        if (hdr_bytes.size() < sizeof(BBMTapeBlockHdr))
            return false;

        // Get block data from BBM Tape Header
        loadAdr = Utility::bytes2uint(&hdr_bytes[BBM_TAPE_BLOCK_LOAD_ADR], 4, true);
        execAdr = Utility::bytes2uint(&hdr_bytes[BBM_TAPE_BLOCK_EXEC_ADR], 4, true);
        size = Utility::bytes2uint(&hdr_bytes[BBM_TAPE_BLOCK_LEN], 2, true);
        no = Utility::bytes2uint(&hdr_bytes[BBM_TAPE_BLOCK_NO], 2, true);
        nextAdr = Utility::bytes2uint(&hdr_bytes[BBM_TAPE_BLOCK_NEXT_ADR], 4, true);
        Byte flags = hdr_bytes[BBM_TAPE_BLOCK_FLAGS];

        if (limitBlockNo)
            // Only use low byte of block no (fix for some programs with the high byte block no being incorrect - usually 0xff)
            no = no & 0xff;

        // Update Tape File Block Header based on flags
        if (!updateFromBBMFlags(flags))
            return false;


    } else if (targetMachine == ACORN_ATOM) {

        // Check that the no of remaining bytes are sufficient to hold the header
        if (hdr_bytes.size() < sizeof(AtomTapeBlockHdr))
            return false;

        // Get block data from Atom Tape Header
        loadAdr = hdr_bytes[ATOM_TAPE_BLOCK_LOAD_ADR_H] * 256 + hdr_bytes[ATOM_TAPE_BLOCK_LOAD_ADR_L];
        execAdr = hdr_bytes[ATOM_TAPE_BLOCK_EXEC_ADR_H] * 256 + hdr_bytes[ATOM_TAPE_BLOCK_EXEC_ADR_L];
        no = hdr_bytes[ATOM_TAPE_BLOCK_NO_H] * 256 + hdr_bytes[ATOM_TAPE_BLOCK_NO_L];
        uint32_t block_sz_M1 = hdr_bytes[ATOM_TAPE_BLOCK_LEN];
        Byte flags = hdr_bytes[ATOM_TAPE_BLOCK_FLAGS]; // b7: not  last block.; b6: block has data; b5:not 1st block.

        if (limitBlockNo)
            // Only use low byte of block no (fix for some programs with the high byte block no being incorrect - usually 0xff)
            no = no & 0xff;

        // Update Tape File Block Header based on flags
        if (!updateFromAtomFlags(flags, block_sz_M1))
            return false;

    }
   
    else
        return false;

    return true;
}

bool FileBlock::updateFromBBMFlags(Byte flags)
{
    if (flags & 0x40) // b6 set for empty block
        size = 0;
    if (no == 0)
        blockType = BlockType::First;
    else
        blockType = BlockType::Other;
    if (flags & 0x80) { // b7 set for last block
        if (no == 0)
            blockType = BlockType::Single;
        else
            blockType = BlockType::Last;
    }
    locked = (flags & 0x1) != 0; // b0 set for locked block
 
    return true;
}

Byte FileBlock::crBBMFlags()
{
    Byte b7 = (lastBlock() ? 0x80 : 0x00);
    Byte b6 = (size == 0 ? 0x40 : 0x00);
    Byte b0 = (locked ? 0x1 : 0x0);

    return (b7 | b6 | b0);
}

bool FileBlock::updateFromAtomFlags(Byte flags, Byte block_sz_M1)
{
    if ((flags & 0x40) == 0x40)
        size = block_sz_M1 + 1;
    else
        size = 0;

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

    return true;
}

Byte FileBlock::crAtomFlags()
{
    Byte b7 = (!lastBlock() ? 0x80 : 0x00); // Bit 7  set if not the last block 
    Byte b6 = (size > 0 ? 0x40 : 0x00); // Bit 6  set if  block contains data.
    Byte b5 = (!firstBlock() ? 0x20 : 0x00); // Bit 5 set if not the first block
    
    return (b7 | b6 | b5);

}

Byte FileBlock::crFlags()
{
    if (targetMachine <= BBC_MASTER)
        return crBBMFlags();
    else
        return crAtomFlags();
}

// Encode Tape File block header into Atom/BBC Micro cassette format block header
bool FileBlock::encodeTapeBlockHdr(Bytes &hdr)
{
    if (targetMachine <= BBC_MASTER) {

        // <name:1-10> <load adr:4> <exec adr:4> <block no:2> <block len:2> <block flag:1> <next adr:4>

        // Write block name
        for (int i = 0; i < name.length() && i < BBM_TAPE_NAME_LEN; hdr.push_back(name[i++]));
        hdr.push_back(0x0); // 0x0 as terminator

        Utility::uint2bytes(loadAdr, hdr, 4, true);     // load adr
        Utility::uint2bytes(execAdr, hdr, 4, true);     // exec adr
        Utility::uint2bytes(no, hdr, 2, true);          // block no
        Utility::uint2bytes(size, hdr, 2, true);        // block len
        Byte flags = crBBMFlags();                      // flags
        hdr.push_back(flags);
        Utility::uint2bytes(0x0, hdr, 4, true);         // next address
    }

    else if (targetMachine == ACORN_ATOM) {
        
        if (size > 256)
            return false;

        //header = <name:2 - 14> <flag:1> <block no:2> <block len -1:1> <exec adr:2> <load adr:2>

        // Write block name
        int name_len = 0;
        for (int i = 0; i < name.length() && i < ATOM_TAPE_NAME_LEN; hdr.push_back(name[i++]));
        hdr.push_back(0xd); // 0xd as terminator

        // Write rest of block
        Byte flags = crAtomFlags();                 // flags
        hdr.push_back(flags);
        hdr.push_back((no >> 8) & 0xff);            // block no
        hdr.push_back(no & 0xff);
        hdr.push_back((size > 0 ? size - 1 : 0));   // block length - 1
        hdr.push_back((execAdr >> 8) & 0xff);       // execution address
        hdr.push_back(execAdr & 0xff);
        hdr.push_back((loadAdr >> 8) & 0xff);       // load address
        hdr.push_back(loadAdr & 0xff);
    }
    
    else
        return false;

    return true;
    
}



bool FileBlock::logFileBlockHdr(ostream* fout)
{
    if (fout == NULL)
        return true;

    
    if (targetMachine <= BBC_MASTER) {
        *fout << setw(16) << setfill(' ') << name << " " <<
            hex << setfill('0') << setw(8) << loadAdr << " " <<
            hex << setfill('0') << setw(9) << loadAdr + 256 * no + size - 1 << " " <<
            hex << setfill('0') << setw(8) << execAdr << " " <<
            hex << setfill('0') << setw(4) << no << " " <<
            hex << setfill('0') << setw(4) << size << " " <<
            hex << setfill('0') << setw(2) << hex << (int)crBBMFlags() << setfill(' ') << " " <<
            setw(8) << _BLOCK_ORDER(blockType);
        if (leadToneCycles != -1 || trailerToneCycles != -1 || blockGap != -1) {
            *fout <<
                " PRELUDE LEAD " << setw(5) << (preludeToneCycles > 0 ? to_string(preludeToneCycles) : "-") << " cycles: " <<
                Utility::roundedPD(" (POSTLUDE) LEAD ", (double)leadToneCycles / F2_FREQ, ": ") <<
                Utility::roundedPD(" TRAILER ", (double)trailerToneCycles / F2_FREQ, ": ") <<
                Utility::roundedPD("GAP ", blockGap, "");
        }
    }
    else if (targetMachine == ACORN_ATOM) {
        *fout << setw(16) << setfill(' ') << name << " " <<
            hex << setfill('0') << setw(4) << loadAdr << " " <<
            hex << setfill('0') << setw(5) << loadAdr + size - 1 << " " <<
            hex << setfill('0') << setw(4) << execAdr << " " <<
            hex << setfill('0') << setw(4) << no << " " <<
            hex << setfill('0') << setw(4) << size << " " << setfill(' ') << " " <<
            hex << setfill('0') << setw(2) << hex << (int)crAtomFlags() << setfill(' ') << " " <<
            setw(8) << _BLOCK_ORDER(blockType);
        if (leadToneCycles != -1 || microToneCycles != -1 || blockGap != -1) {
            *fout << Utility::roundedPD(" LEAD TONE ", (double)leadToneCycles / F2_FREQ, ": ") <<
                Utility::roundedPD("MICRO TONE ", (double)microToneCycles / F2_FREQ, ": ") <<
                Utility::roundedPD("GAP ", blockGap, "");
        }

    }
    else
        return false;

    if (this->locked)
        *fout << " L";
    else
        *fout << " U";

    if (this->tapeStartTime < 0)
        *fout << "\n";
    else
        *fout << " " << Utility::encodeTime(this->tapeStartTime) << " => " << Utility::encodeTime(this->tapeEndTime) << "\n";

    return true;
}

bool FileBlock::logFileBlockHdr()
{
    return logFileBlockHdr(&cout);
}

void TapeFile::logFileHdr()
{
    logFileHdr(&cout);
}

void TapeFile::logFileHdr(ostream* fout)
{
    if (fout == NULL)
        return;

    if (header.targetMachine <= BBC_MASTER) {
        uint32_t n_blocks = (uint32_t)blocks.size();
        *fout << setw(16) << header.name << " " <<
            hex << setfill('0') << setw(8) << header.loadAdr << " " <<
            hex << setfill('0') << setw(9) << (header.size == 0 ? header.loadAdr : header.loadAdr + header.size - 1) << " " <<
            hex << setfill('0') << setw(8) << header.execAdr << " " <<
            dec << setfill(' ') << setw(5) << n_blocks << " " <<
            hex << setfill('0') << setw(4) << header.size;
    }
    else if (header.targetMachine == ACORN_ATOM) {
        uint32_t n_blocks = (uint32_t)blocks.size();
        *fout << setw(16) << header.name << " " <<
            hex << setfill('0') << setw(4) << header.loadAdr << " " <<
            hex << setfill('0') << setw(5) << (header.size==0? header.loadAdr :header.loadAdr + header.size - 1) << " " <<
            hex << setfill('0') << setw(4) << header.execAdr << " " <<
            dec << setfill(' ') << setw(5) << n_blocks;
    }
    
    else {
    }

    if (this->header.locked)
        *fout << " L";
    else
        *fout << " U";

    if (tapeStartTime < 0)
        *fout << setfill(' ') << "\n";
    else
        *fout << setfill(' ') << " " << Utility::encodeTime(tapeStartTime) << " => " << Utility::encodeTime(tapeEndTime) << "\n";

}

bool FileBlock::encodeTAPHdr(
    string tapefileName, uint32_t fileLoadAdr, uint32_t LA, uint32_t EA, uint32_t blockNo, uint32_t BlockSz
)
{
    name = tapefileName;
    loadAdr = LA;
    execAdr = EA;
    no = blockNo;
    size = BlockSz;
    locked = false;
    
    if (no == 0)
        blockType = BlockType::First;
    else
        blockType = BlockType::Other;


    return true;
}

string TapeFile::crValidDiscFilename(string programName)
{
    return crValidDiscFilename(header.targetMachine, programName);
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
        if (s.length() < 7 && c > (char) 0x20 && c <= (char) 0x7e && invalid_chars.find(c) == string::npos)
            s += c;
    }

    return s;
}

string TapeFile::crValidBlockName(string fileName)
{
    return crValidBlockName(header.targetMachine, fileName);
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