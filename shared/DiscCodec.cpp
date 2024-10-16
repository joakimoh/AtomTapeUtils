#include "DiscCodec.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>
#include <cstdint>
#include "Utility.h"
#include <cmath>

DiscCodec::DiscCodec(Logging logging) : mVerbose(logging.verbose)
{
}

bool DiscCodec::move2File(int discSide, int sectorNo)
{
    int sector_no = sectorNo % NO_OF_SECTORS_PER_TRACK;
    int track_no = sectorNo / NO_OF_SECTORS_PER_TRACK;
    return move(discSide, track_no, sector_no, 0);
}

bool DiscCodec::move(int discSide, int trackNo, int sectorNo, int byteOffset)
{
    DiscPos disc_pos(discSide, trackNo, sectorNo, byteOffset);

    if (mFin_p == NULL || mDisc_p == NULL || mDisc_p->side.size() == 0 || (discSide == 1 && mDisc_p->side.size() == 1)) {
        cout << "Illegal disc position " << disc_pos.toStr() << "\n";
        return false;
    }

    int max_n_tracks = MAX_NO_OF_TRACKS_PER_SIDE;
    if (
        discSide < 0 || discSide > 1 ||
        trackNo < 0 || trackNo >= max_n_tracks ||
        sectorNo < 0 || sectorNo >= NO_OF_SECTORS_PER_TRACK ||
        byteOffset < 0 || byteOffset >= SECTOR_SIZE
        ) {
        cout << "Illegal disc position " << disc_pos.toStr() << "\n";
        return false;
    }

    if (discSide == 1 && mDisc_p->side.size() != 2) {
        return false;
    }

    int pos;
    if (discSide == 1)
        pos = mDisc_p->side[0].discSize + trackNo * NO_OF_SECTORS_PER_TRACK * SECTOR_SIZE + sectorNo * SECTOR_SIZE + byteOffset;
    else
        pos = trackNo * NO_OF_SECTORS_PER_TRACK * SECTOR_SIZE + sectorNo * SECTOR_SIZE + byteOffset;
    if (!Utility::move2FilePos(*mFin_p, (streampos) pos)) {
        cout << "Failed to move to " << disc_pos.toStr() << "\n";
        return false;
    }

    mDiscPos.side = discSide;
    mDiscPos.track = trackNo;
    mDiscPos.sector = sectorNo;
    mDiscPos.byte = byteOffset;

    return true;
}

bool DiscCodec::readByte(Byte& byte)
{
    if (!Utility::readBytes(*mFin_p, (Byte*) &byte, 1)) {
        return false;
    }
    mDiscPos.byte += 1;

    return true;
}

bool DiscCodec::readBytes(Byte* bytes, int n)
{
    if (!Utility::readBytes(*mFin_p, bytes, n)) {
        return false;
    }
    mDiscPos.byte += n;
 
    return true;
}

bool DiscCodec::readBytes(Bytes &bytes, int n)
{
    for (int i = 0; i < n; i++) {
        Byte b;
        if (!Utility::readBytes(*mFin_p, (Byte*) &b, 1)) {
            return false;
        }
        bytes.push_back(b);
    }
    mDiscPos.byte += n;

    return true;
}

//
// Read up to 64 bits from up to 8 bytes.
// The high bit is at offset highBitOffset of the FIRST (MSB) byte read
//
bool DiscCodec::readBits(int highBitOffset, int nBits, uint64_t &bits)
{
    if (highBitOffset <= 0 || highBitOffset >= 8 || nBits <= 0 || nBits > 64) {
        cout << "Invalid parameters\n";
        return false;
    }

    int high_bit = highBitOffset;
    int remaining_bits = nBits;
    bits = 0;
    int n_bytes = (int)ceil(nBits / 8) + 1;
    for (int b = 0; b < n_bytes ; b++) {
        bits = bits << 8;
        Byte d;
        if (!readByte(d)) {
            return false;
        }
        int mask_bits;
        int mask;
        if (high_bit + 1 <= remaining_bits) { // read high_bit::b0
            mask_bits = high_bit + 1; // no of bits to extract from byte
            mask = (1 << mask_bits) - 1; // extraction mask
        } else  { // read high_bit::high_bit-remaining_bits+1
            mask_bits = remaining_bits; // no of bits to extract from byte
            mask = ((1 << mask_bits) - 1) << (high_bit - remaining_bits + 1); // extraction mask
        }
            
        remaining_bits -= mask_bits;
        if (remaining_bits >= 8)
            high_bit = 7;
        else
            high_bit = highBitOffset;
         bits |= (d & mask);

    }

    return true;
}

bool DiscCodec::openDiscFile(string discPath, Disc &disc)
{
    mDisc_p = &disc;
    mFin_p = new ifstream(discPath, ios::in | ios::binary | ios::ate);

    if (!*mFin_p) {
        cout << "couldn't open file " << discPath << "\n";
        return false;
    }

    return true;
}

bool DiscCodec::closeDiscFile()
{
    if (mFin_p != NULL) {
        mFin_p->close();
        delete mFin_p;
    }

    return true;
}

bool DiscCodec::read(string discPath, Disc& disc)
{

    if (!openDiscFile(discPath, disc)) {
        cout << "couldn't open file " << discPath << "\n";
        return false;
    }

    // Get file size
    mFin_p->seekg(0, ios::end);
    streamsize file_sz = mFin_p->tellg();

    // Read volume title (sector 0 bytes 0 to 7 + sector 1 bytes 0 to 3)
    DiscSide discSide;

    // Read one to two sides of the disc
    bool two_sides = true;
    for (int side_no = 0; side_no < 2 && two_sides; side_no++) {


        disc.side.push_back(discSide);

        // Read volume title
        // Space or null-padded ASCII chars - stored in sector 0 bytes 0 to 7 + sector 1 bytes 0 to 3
        Byte title[12];
        if (!move(side_no, 0,0,0) || !readBytes((Byte*)&title[0], 8)) { // sector 0 bytes 0 to 7
            cout << "Failed to read first eight characters of the volume title\n";
            closeDiscFile();
            return false;
        }
        if (!move(side_no, 0,1,0) || !readBytes((Byte*)&title[8], 4)) { // sector 1 bytes 0 to 3
            cout << "Failed to read last four characters of the volume title\n";
            closeDiscFile();
            return false;
        }
        disc.side[side_no].discTitle = Utility::paddedCharArray2String(title, 12);

        if (mVerbose)
            cout << "Volume title of side #" << side_no << ": '" << disc.side[side_no].discTitle << "'\n";

        // Get cycle no
        Byte cycle_no; // BDC-coded version of the disc (0-99) - stored in sector 1 byte 4
        if (!readByte(cycle_no)) {
            cout << "Failed to read cycle no for volume '" << disc.side[side_no].discTitle << "'\n";
            closeDiscFile();
            return false;
        }
        disc.side[side_no].cycleNo = cycle_no;

        if (mVerbose)
            cout << "Cycle no for volume '" << disc.side[side_no].discTitle << "': " << cycle_no << "\n";

        // Get no of files
        Byte file_offset; // offset to the last valid file entry in each sector - stored in sector 1 byte 5
        if (!readByte(file_offset)) {
            cout << "Failed to read file offset for volume '" << disc.side[side_no].discTitle << "'\n";
            return false;
        }      
        disc.side[side_no].nFiles = file_offset / 8;

        if (mVerbose)
            cout << "No of files for volume '" << disc.side[side_no].discTitle << "': " <<
            (int) disc.side[side_no].nFiles << "\n";

        // Get boot option
        uint64_t boot_option; // //  byte 6 b5b4 in sector 1
        if (!readBits(5,2, boot_option)) {
            cout << "Failed to read file offset for volume '" << disc.side[side_no].discTitle << "'\n";
            return false;
        }
        disc.side[side_no].bootOption = BootOption(boot_option & 0x3);

        if (mVerbose)
            cout << "Boot option for volume '" << disc.side[side_no].discTitle << "': " << 
            _BOOT_OPTION(disc.side[side_no].bootOption) << "\n";

        // Get no of sectors - b7:b0 in sector 1 byte 7, b9b8 in b1b0 of sector 1 byte 6
        uint64_t bits;
        if (!move(side_no, 0, 1, 6)) { // Move to sector 1, byte 6
            closeDiscFile();
            return false;
        }
        if (!readBits(1, 10, bits)) { // Read 10 bits, starting from byte 6
            closeDiscFile();
            cout << "Failed to read no of sectors for volume '" << disc.side[side_no].discTitle << "'\n";
            return false;
        }
        disc.side[side_no].discSize = (Word) bits;

        // Now that the size of side #0 is known, check whether the file has room for a second side
        if (side_no == 0 && file_sz <= mDisc_p->side[0].discSize * SECTOR_SIZE)
            two_sides = false; // No room for a second side


        if (mVerbose)
            cout << "No of sectors for volume '" << disc.side[side_no].discTitle << "': " << (int)disc.side[side_no].discSize << "\n";

        // Read the name and directory belonging of each file stored 
        if (!move(side_no, 0, 0, 8 )) { // Move to track 0, sector 0, byte file_no * 8
            closeDiscFile();
            return false;
        }
        for (int file_no = 0; file_no < disc.side[side_no].nFiles; file_no++) {

            Byte file_name[7];
            if (!readBytes((Byte*)file_name, 7)) {
                cout << "Failed to read file name at " << mDiscPos.toStr() << "\n";
                return false;
            }
            DiscFile file;
            file.name = Utility::paddedCharArray2String(file_name, 7);
            Byte dir_name;
            if (!readByte(dir_name)) {
                cout << "Failed to read directory name for file '" << file.name << " at " << mDiscPos.toStr() << "\n";
                return false;
            }
            DiscDirectory d;
            d.name = (char) (dir_name & 0x7f);
            file.dir = d.name;
            file.locked = ((dir_name & 0x80) != 0 ? true : false);
            if (disc.side[side_no].directory.find(d.name) == disc.side[side_no].directory.end())
                disc.side[side_no].directory[d.name] = d;
            disc.side[side_no].directory[d.name].file.push_back(file);
            disc.side[side_no].files.push_back(file);
        }

        // Read remaining information about each file stored on the track
        for (int file_no = 0; file_no < disc.side[side_no].nFiles; file_no++) {

            if (!move(side_no, 0, 1, 8 + file_no * 8)) { // Move to track 0, sector 0, byte file_no * 8
                closeDiscFile();
                return false;
            }

            Byte file_info[8];
            DiscFile &file = disc.side[side_no].files[file_no];
            if (!readBytes((Byte*)&file_info[0], 8)) {
                cout << "Failed to read information about file " << file.name << " at " << mDiscPos.toStr() << "\n";
                return false;
            }
            file.loadAdr = Utility::bytes2uint(&file_info[0], 2, true) + (((file_info[6] >> 2) & 0x3) << 16);
            file.execAdr = Utility::bytes2uint(&file_info[2], 2, true) + (((file_info[6] >> 6) & 0x3) << 16);
            file.size = Utility::bytes2uint(&file_info[4], 2, true) + (((file_info[6] >> 4) & 0x3) << 16);
            file.startSector = file_info[7] + ((file_info[6] & 0x3) << 8);

            if (mVerbose) {
                cout << 
                    "Dir '" << setw(1) << file.dir <<
                    "': File '" << setw(12) << file.name <<
                    "' LA: " << setw(5) << setfill('0') << hex << file.loadAdr <<
                    ", EA: " << setw(5) << setfill('0') << hex << file.execAdr <<
                    ", SZ: " << setw(5) << setfill('0') << hex << file.size << setfill(' ') <<
                    ", LOCKED: " << setw(5) << (file.locked ? "true" : "false") <<
                    ", Sector: " << setw(3) << file.startSector <<
                    "\n";
            }

            move2File(side_no, file.startSector);

            if (!readBytes(file.data, file.size)) {
                cout << "Failed to read data for file!\n";
                return false;
            }
   
         }

    }

    if (!closeDiscFile()) {
        return false;
    }

    return true;
}

bool DiscCodec::write(string title, string discPath, vector<TapeFile> &tapeFiles)
{
    if (tapeFiles.size() == 0 || tapeFiles.size() > 31) {
        cout << "No of files to write to image was " << tapeFiles.size() << " but must be between 1 and 31!\n";
        return false;
    }

    for (int f = 0; f < tapeFiles.size(); f++) {
        if (tapeFiles[f].blocks.size() == 0) {
            cout << "At least one file to write to image was empty!\n";
            return false;
        }
    }

    // single density, 80 tracks, 10 sectors per track, 256 bytes per sector
    char zero_sector[256] = { 0 };
    char zero_block[8] = { 0 };
    char z = 0x0;
    const int block_size = 8;
    const int sector_size = 256; 
    const int no_of_tracks = 80;
    const int sectors_per_track = 10;

    if (mVerbose)
        cout << tapeFiles.size() << " files to be written into disc image '" << discPath << "' with volume name '" << title << "'\n";

    // Create disc image file for writing
    ofstream fout(discPath, ios::out | ios::binary | ios::ate);
    if (!fout) {
        cout << "can't write to disc image file " << discPath << "\n";
        return false;
    }

    // Write first 8 chars of Volume title (in Sector 0)
    for (int i = 0;  i < 8; i++) {
        char c = ' ';
        if (i < title.length())
            c = title[i];
        if (!fout.write((char*)&c, sizeof(c))) {
            cout << "Failed to write first 8 chars of volume title\n";
            return false;
        }
    }

    // Write file names and their directories (in Sector 0)
    for (int f = 0; f < tapeFiles.size(); f++) {
        cout << "File #" << f << " '" << tapeFiles[f].validFileName << "'\n";
        for (int i = 0; i < 7; i++) {
            char c = ' ';
            if (i < tapeFiles[f].validFileName.size())
                c = tapeFiles[f].validFileName[i];
            if (!fout.write((char*)&c, sizeof(c))) {
                cout << "Failed to write filename '" << tapeFiles[f].validFileName << "'\n";
                return false;
            }
        }
        char d = '+'; // Just have all files in one directory '+'
        if (!fout.write((char*)&d, sizeof(d))) {
            cout << "Failed to write directory '" << d << "'\n";
            return false;
        }
    }

    // Clear remaining part of Sector 0
    for (int f = tapeFiles.size() * block_size + block_size; f < sector_size; f++) {
         if (!fout.write((char*)&z, sizeof(z))) {
                cout << "Failed to clear sector 0\n";
                return false;
        }
    }

    cout << "Bytes written after sector 0 : " << fout.tellp() << "\n";

    // Write last 4 chars of Volume title (in Sector 1)
    for (int i = 0; i < 4; i++) {
        char c = ' ';
        if (i + 8 < title.length())
            c = title[i+8];
        if (!fout.write((char*)&c, sizeof(c))) {
            cout << "Failed to write last 4 chars of volume title\n";
            return false;
        }
    }

    // Write cycle no
    char cycle_no = 0x0;
    if (!fout.write((char*)&cycle_no, sizeof(cycle_no))) {
        cout << "Failed to write cycle no\n";
        return false;
    }

    // Write no of files
    char file_offset = (char) (tapeFiles.size() * 8);
    if (!fout.write((char*)&file_offset, sizeof(file_offset))) {
        cout << "Failed to write cycle no\n";
        return false;
    }

    // Write boot option (00 <=> No action) and no of sectors
    Word no_of_sectors = no_of_tracks * sectors_per_track; 
    Byte boot_byte = (0x00 << 4) | ((no_of_sectors >> 8) & 0x3);
    Byte sectors_byte = no_of_sectors & 0xff;
    if (!fout.write((char*)&boot_byte, sizeof(boot_byte)) || !fout.write((char*)&sectors_byte, sizeof(sectors_byte))) {
        cout << "Failed to write boot option or no of sectors\n";
        return false;
    }
 
    // Write remaining file information (in Sector 1)
    uint32_t next_available_sector = 2;
    for (int f = 0; f < tapeFiles.size(); f++) {
        FileBlock &first_block = tapeFiles[f].blocks[0];
        Byte file_info[8];
        uint32_t start_sector = next_available_sector;
        next_available_sector += (int) ceil((double) tapeFiles[f].size() / sector_size);

        cout << "File " << first_block.blockName() << " starts at Sector " << start_sector << " and occupies " <<
            next_available_sector - start_sector << " sectors\n";

        Byte load_adr_b9b10 = (first_block.loadAdr() >> (32 - 2)) & 0xc0;
        Byte exec_adr_b9b10 = (first_block.execAdr() >> (32 - 6)) & 0xc0;
        Byte start_sector_b8b9 = (start_sector >> 8) & 0x3;

        file_info[0] = first_block.loadAdr() & 0xff;
        file_info[1] = (first_block.loadAdr() >> 8) & 0xff;
        file_info[2] = first_block.execAdr() & 0xff;
        file_info[3] = (first_block.execAdr() >> 8) & 0xff;
        file_info[4] = tapeFiles[f].size() & 0xff;
        file_info[5] = (tapeFiles[f].size() >> 8) & 0xff;
        file_info[6] = load_adr_b9b10 | exec_adr_b9b10 | start_sector_b8b9;
        file_info[7] = start_sector & 0xff;

        if (!fout.write((char*)&file_info[0], sizeof(file_info))) {
                cout << "Failed to write info for file '" << first_block.blockName() << "'\n";
                return false;
            }

    }

    // Clear remainding part of sector 1
    for (int f = tapeFiles.size() * block_size + block_size; f < sector_size; f++) {
        if (!fout.write((char*)&z, sizeof(z))) {
            cout << "Failed to clear sector 0\n";
            return false;
        }
    }

    cout << "Bytes written after file info : " << fout.tellp() << "\n";

    // Write the file data
    for (int f = 0; f < tapeFiles.size(); f++) {

        vector<FileBlock> &blocks = tapeFiles[f].blocks;

        cout << "File " << tapeFiles[f].validFileName << " with #" << blocks.size() << " blocks\n";

        for (int b = 0; b < blocks.size(); b++) {

            cout << "Block " << b << " with " << blocks[b].data.size() << " bytes\n";

            for (int d = 0; d < sector_size; d++) {

                Byte c;
                if (d < blocks[b].data.size())
                    c = blocks[b].data[d];
                else
                    c = 0x0;

                if (!fout.write((char*)&c, sizeof(c))) {
                    cout << "Failed to write data for block " << b << " of file '" << blocks[b].blockName() << "'\n";
                    return false;
                }
            }
        }
    }

    cout << "Used sectors: " << next_available_sector << ", written bytes so far: " << fout.tellp() << "\n";

    // Clear remaining sectors
    for (int sector = next_available_sector; sector < no_of_sectors; sector++) {
        if (!fout.write((char*)&zero_sector[0], sizeof(zero_sector))) {
            cout << "Failed to clear unused sectors!\n";
            return false;
        }
    }

    cout << "Done - " << fout.tellp() << " bytes written!\n";

    // Close disc image file
    fout.close();

   

    return true;
}