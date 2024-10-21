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

    if (mVerbose)
        cout << dec;

    // Check the format
    int n_sides;
    string file_ext = Utility::getFileExt(discPath);
    if (file_ext == ".ssd") {
        n_sides = 1;
        if (mVerbose)
            cout << "Single-sided format SSD detected\n";
    }
    else if (file_ext == ".dsd") {
        n_sides = 2;
        if (mVerbose)
            cout << "Single-sided interleaved format DSD detected\n";
    }
    else {
        cout << "Unsupported disc format '" << file_ext << "'\n";
        return false;
    }

    // Get file size
    mFin_p->seekg(0, ios::end);
    streamsize file_sz = mFin_p->tellg();
    mFin_p->seekg(0);

    // Constants for single density, 80 tracks, 10 sectors per track, 256 bytes per sector
    const int block_size = 8;
    const int sector_size = 256;
    const int no_of_tracks = 80; // could sometimes be 40 but assumed 80 to start with
    const int sectors_per_track = 10;
    const int track_size = sectors_per_track * sector_size;

    // Read disc image into one or two byte vectors (one per disc side)
    // The disc image should really be complete but as sometimes
    // it is not (sectors without data are not written to the disc image)
    // we need to tolerate this and stop even if the exected no of tracks
    // have not been read. Will also allow #tracks to be different
    // (like 40 instead of 80).
    Bytes side_image_bytes[2];
    Byte track_bytes[track_size];
    bool eof = false;
    int track = 0;
    if (n_sides == 2) {
        for (track = 0; !eof && track < no_of_tracks*2; track++) {
            if (!mFin_p->read((char*) &track_bytes[0], track_size)) {
                
                eof = true;
            }
            else {
                if (track % 2 == 0)
                    side_image_bytes[0].insert(side_image_bytes[0].end(), track_bytes, track_bytes + track_size);
                else
                    side_image_bytes[1].insert(side_image_bytes[1].end(), track_bytes, track_bytes + track_size);
            }
        }
    } else {
        for (track = 0; !eof && track < no_of_tracks; track++) {
            if (!mFin_p->read((char*)&track_bytes[0], track_size)) {
                eof = true;
            }
            else {
                side_image_bytes[0].insert(side_image_bytes[0].end(), track_bytes, track_bytes + track_size);
            }
        }
    }
    if (!closeDiscFile())
            return false;

    if (mVerbose)
        cout << dec << track << " tracks read\n";

    // Read one to two sides of the disc
    for (int side_no = 0; side_no < n_sides; side_no++) {

        DiscSide ds;
        disc.side.push_back(ds);
        DiscSide &discSide = disc.side[side_no];

        Bytes &image_bytes = side_image_bytes[side_no];
        BytesIter image_bytes_iter = image_bytes.begin();

        // Read first 8 chars of volume title from sector 0 bytes 0 to 7
        // Space or null-padded ASCII chars
        Byte title[12];
        if (!Utility::readBytes(image_bytes_iter, image_bytes, &title[0], 8)) { 
            cout << "Failed to read first eight characters of the volume title\n";
            return false;
        }

        // Jump to next sector
        if (image_bytes_iter + sector_size - 8 >= image_bytes.end())
            return false;
        image_bytes_iter += sector_size - 8;

        // Get first block of sector 1
        Byte catalogue_info[8];
        if (!Utility::readBytes(image_bytes_iter, image_bytes, &catalogue_info[0], 8)) {
            cout << "Failed to read first catalogue block of sector 0\n";
            return false;
        }

;        // Read last 4 chars of volume title from sector 1 bytes 0 to 3
        copy(&catalogue_info[0], &catalogue_info[4], &title[8]);
        discSide.discTitle = Utility::paddedByteArray2String(title, 12);
        if (mVerbose)
            cout << "Volume title of side #" << side_no << ": '" << discSide.discTitle << "'\n";

        // Get cycle no; BDC-coded version of the disc (0-99) - stored in sector 1 byte 4
        discSide.cycleNo = catalogue_info[4];

        if (mVerbose)
            cout << "Cycle no for volume '" << discSide.discTitle << "': " << (int) discSide.cycleNo << "\n";

        // Get no of files; offset to the last valid file entry in each sector - stored in sector 1 byte 5
        discSide.nFiles = catalogue_info[5] / 8;

        if (mVerbose)
            cout << "No of files for volume '" << discSide.discTitle << "': " <<
            (int) discSide.nFiles << "\n";

        // Get boot option - byte 6 b5b4 in sector 1
        discSide.bootOption = BootOption((catalogue_info[6] >> 4) & 0x3);

        if (mVerbose)
            cout << "Boot option for volume '" << discSide.discTitle << "': " << 
            _BOOT_OPTION(discSide.bootOption) << "\n";

        // Get no of sectors - b7:b0 in sector 1 byte 7, b9b8 in b1b0 of sector 1 byte 6
        discSide.discSize = (Word) catalogue_info[7] | (Word) ((catalogue_info[6] & 0x3) << 8);

        if (mVerbose)
            cout << "No of sectors for volume '" << discSide.discTitle << "': " << (int)discSide.discSize << "\n";

        // Jump back to Sector 0, Block 1
        image_bytes_iter = image_bytes.begin() + block_size;

        // Read the name and directory belonging of each file stored 
        for (int file_no = 0; file_no < discSide.nFiles; file_no++) {

            Byte file_name[7];
            if (!Utility::readBytes(image_bytes_iter, image_bytes, &file_name[0], 7)) {
                cout << "Failed to read file name\n";
                return false;
            }
            DiscFile file;
            file.name = Utility::paddedByteArray2String(file_name, 7);
            Byte dir_name;
            if (!Utility::readBytes(image_bytes_iter, image_bytes, &dir_name, 1)) {
                cout << "Failed to read directory name for file '" << file.name << "\n";
                return false;
            }
            DiscDirectory d;
            d.name = (char) (dir_name & 0x7f);
            file.dir = d.name;
            file.locked = ((dir_name & 0x80) != 0 ? true : false);
            if (discSide.directory.find(d.name) == discSide.directory.end())
                discSide.directory[d.name] = d;
            discSide.directory[d.name].file.push_back(file);
            discSide.files.push_back(file);
        }

        // Read remaining information about each file stored on the track
        for (int file_no = 0; file_no < discSide.nFiles; file_no++) {

            // Jump to Sector 1, Block (1 + file_no)
            if (image_bytes.begin() + sector_size + block_size * (1 + file_no) >= image_bytes.end())
                return false;
            image_bytes_iter = image_bytes.begin() + sector_size + block_size * (1 + file_no);

            Byte file_info[8];
            DiscFile &file = discSide.files[file_no];
            if (!Utility::readBytes(image_bytes_iter, image_bytes, &file_info[0], 8)) {
                cout << "Failed to read information about file " << file.name << "\n";
                return false;
            }
            // Load adress: Sector 1 b15:0 in byte 8-9 + b17:16 in b3b2 of sector 1 byte 14
            file.loadAdr = Utility::bytes2uint(&file_info[0], 2, true) + (((file_info[6] >> 2) & 0x3) << 16);
            // Exec adress: Sector 1 b15:0 in byte 10-11 + b17:16 in b7b6 of sector 1 byte 14
            file.execAdr = Utility::bytes2uint(&file_info[2], 2, true) + (((file_info[6] >> 6) & 0x3) << 16);
            // Size: Sector 1 b15:0 in byte 12-13 + b17:16 in b5b4 of sector 1 byte 14
            file.size = Utility::bytes2uint(&file_info[4], 2, true) + (((file_info[6] >> 4) & 0x3) << 16);
            // Start sector: Sector 1 b7:0 in byte 15 + b17:16 in b1b0 of sector 1 byte 14
            file.startSector = file_info[7] + ((file_info[6] & 0x3) << 8);

            if (mVerbose) {
                cout << 
                    "Dir '" << setw(1) << file.dir <<
                    "': File '" << setw(12) << file.name <<
                    "' LA: " << setw(5) << setfill('0') << hex << file.loadAdr <<
                    ", EA: " << setw(5) << setfill('0') << hex << file.execAdr <<
                    ", SZ: " << setw(5) << setfill('0') << hex << file.size << setfill(' ') <<
                    ", LOCKED: " << setw(5) << (file.locked ? "true" : "false") <<
                    ", Sector: " << setw(3) << file.startSector << dec <<
                    "\n";
            }

            // Jump to start sector for file
            if (image_bytes.begin() + file.startSector * sector_size >= image_bytes.end())
                return false;
            image_bytes_iter = image_bytes.begin() + file.startSector * sector_size;

            if (!Utility::readBytes(image_bytes_iter, image_bytes, file.data, file.size)) {
                cout << "Failed to read data for file!\n";
                return false;
            }

            if (mVerbose)
                cout << "Read " << file.data.size() << " bytes out of expected " << file.size << " for file '" << file.name << "'\n";
   
         }

    }

    return true;
}

bool DiscCodec::write(string title, string discPath, vector<TapeFile> &tapeFiles)
{
    if (tapeFiles.size() == 0 || tapeFiles.size() > 62) {
        cout << "No of files to write to image was " << tapeFiles.size() << " but must be between 1 and 62!\n";
        return false;
    }

    
    for (int f = 0; f < tapeFiles.size(); f++) {
        if (tapeFiles[f].blocks.size() == 0) {
            cout << "At least one file to write to image was empty!\n";
            return false;
        }
    }

    filesystem::path file_path = discPath;
    string file_path_no_ext = file_path.stem().string();
    string disc_path;
    if (tapeFiles.size() <= 31)
        disc_path = file_path_no_ext + ".ssd";
    else
        disc_path = file_path_no_ext + ".dsd";

    if (mVerbose)
        cout << dec;

    // Create disc image file for writing
    ofstream fout(disc_path, ios::out | ios::binary | ios::ate);
    if (!fout) {
        cout << "can't write to disc image file " << disc_path << "\n";
        return false;
    }

    if (mVerbose)
        cout << tapeFiles.size() << " files to be written into disc image '" << discPath << "' with volume name '" << title << "'\n";

    int n_sides = 1;
    if (tapeFiles.size() > 31) {
        if (mVerbose)
            cout << "Double-sided interleaved disc image will be created to fit all the files\n";
        n_sides = 2;
    }
    else {
        if (mVerbose)
            cout << "Single-sided disc will be created for the files\n";
    }

    // single density, 80 tracks, 10 sectors per track, 256 bytes per sector
    Byte zero_sector[256] = { 0 };
    Byte zero_block[8] = { 0 };
    Byte z = 0x0;
    const int block_size = 8;
    const int sector_size = 256;
    const int no_of_tracks = 80;
    const int sectors_per_track = 10;

    //
    // Create either one side (#files <= 31) or two sides (#files >= 32 && <= 62)
    // but only as one or two byt vectors to be able to interleave two sides later on

    int first_file_no, n_files_side;
    streampos start_file_pos = 0;
    Bytes side_image_bytes[2];
    for (int side = 0; side < n_sides; side++) {

        if (mVerbose)
            cout << "Writing side #" << side << " of the disc\n";

        if (n_sides == 1) { // && side == 0
            first_file_no = 0;
            n_files_side = (int) tapeFiles.size();
        }
        else if (side == 0) {
            first_file_no = 0;
            n_files_side = 31;
        }
        else { // side == 1
            first_file_no = 31;
            n_files_side = (int) tapeFiles.size()-31;
        }

        // Write first 8 chars of Volume title (in Sector 0)
        for (int i = 0; i < 8; i++) {
            Byte c = 0x20;
            if (i < title.length())
                c = (Byte) title[i];
            side_image_bytes[side].push_back(c);
        }

        // Write file names and their directories (in Sector 0)
        for (int f = first_file_no; f < first_file_no+n_files_side; f++) {
            string disc_filename = tapeFiles[f].crValidDiscFilename(tapeFiles[f].programName);
            if (mVerbose)
                cout << "File #" << f << " '" << tapeFiles[f].programName << "' (stored as disc file '" << disc_filename << "')\n";
            for (int i = 0; i < 7; i++) {
                Byte c = 0x20;
                if (i < disc_filename.size())
                    c = (Byte) disc_filename[i];
                side_image_bytes[side].push_back(c);

            }
            Byte d = (Byte) '+'; // Just have all files in one directory '+'
            side_image_bytes[side].push_back(d);
        }

        // Clear remaining part of Sector 0
        for (int f = n_files_side * block_size + block_size; f < sector_size; f++) {
            side_image_bytes[side].push_back(z);
        }

        if (mVerbose)
            cout << side_image_bytes[side].size() - start_file_pos << " bytes written for Sector 0 and side #" << side << "...\n";

        // Write last 4 chars of Volume title (in Sector 1)
        for (int i = 0; i < 4; i++) {
            Byte  c = 0x20;
            if (i + 8 < title.length())
                c = (Byte) title[i + 8];
            side_image_bytes[side].push_back(c);
        }

        // Write cycle no
        Byte cycle_no = 0x0;
        side_image_bytes[side].push_back(cycle_no);


        // Write no of files
        Byte  file_offset = n_files_side * 8;
        side_image_bytes[side].push_back(file_offset);


        // Write boot option (00 <=> No action) and no of sectors
        Word no_of_sectors = no_of_tracks * sectors_per_track;
        Byte boot_byte = (0x00 << 4) | ((no_of_sectors >> 8) & 0x3);
        Byte sectors_byte = no_of_sectors & 0xff;
        side_image_bytes[side].push_back(boot_byte);
        side_image_bytes[side].push_back(sectors_byte);


        // Write remaining file information (in Sector 1)
        uint32_t next_available_sector = 2;
        for (int f = first_file_no; f < first_file_no+n_files_side; f++) {
            FileBlock& first_block = tapeFiles[f].blocks[0];
            Byte file_info[8];
            uint32_t start_sector = next_available_sector;
            next_available_sector += (int)ceil((double)tapeFiles[f].size() / sector_size);

            if (mVerbose)
                cout << "Program " << first_block.blockName() << " starts at Sector " << start_sector << " and occupies " <<
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

            side_image_bytes[side].insert(side_image_bytes[side].end(), file_info, file_info + block_size);


        }

        // Clear remaining part of sector 1
        for (int f = n_files_side * block_size + block_size; f < sector_size; f++) {
            side_image_bytes[side].push_back(z);
        }

        if (mVerbose)
            cout << side_image_bytes[side].size() - start_file_pos << " bytes written for Sector 1 and side #" << side << "...\n";

        // Write the file data
        for (int f = first_file_no; f < first_file_no+n_files_side; f++) {

            vector<FileBlock>& blocks = tapeFiles[f].blocks;

            if (mVerbose)
                cout << "Writing block data for program " << tapeFiles[f].programName << " with #" << blocks.size() << " blocks\n";

            int data_sz = 0;
            for (int b = 0; b < blocks.size(); b++) {

                if (mVerbose)
                    cout << "Block " << b << " with " << blocks[b].data.size() << " bytes\n";

                for (int d = 0; d < blocks[b].data.size(); d++)
                    side_image_bytes[side].push_back(blocks[b].data[d]);

                data_sz += (int) blocks[b].data.size();
            }
            if (data_sz % sector_size != 0) {
                // Fill empty part of last sector with zeroes
                for(int d = 0; d < sector_size - data_sz % sector_size; d++)
                    side_image_bytes[side].push_back(z);
            }
            
        }

        // Clear remaining sectors
        for (int sector = next_available_sector; sector < no_of_sectors; sector++) {
            side_image_bytes[side].insert(side_image_bytes[side].end(), zero_sector, zero_sector + sector_size);
        }

        if (mVerbose)
            cout << side_image_bytes[side].size() - start_file_pos << " bytes written in total for side " << side << "..\n";
    

    }

    // Write image bytes to file
    if (n_sides == 1) {
        BytesIter bi = side_image_bytes[0].begin();
        while (bi < side_image_bytes[0].end()) {
            if (!fout.write((char *)  &(*bi++), 1)) {
                cout << "Failed to write image bytes to file\n";
                fout.close();
                return false;
            }
        }
    }
    else {
        BytesIter bi_0 = side_image_bytes[0].begin();
        BytesIter bi_1 = side_image_bytes[1].begin();
        for (int track = 0; track < no_of_tracks; track++) {
            if (track % 2 == 0) {
                // Write all sectors of one track for side #0
                for (int sector_bytes = 0; sector_bytes < sectors_per_track * sector_size; sector_bytes++) {
                    if (!(bi_0 < side_image_bytes[0].end()) || !fout.write((char*)&(*bi_0++), 1)) {
                        cout << "Failed to write image bytes to file\n";
                        fout.close();
                        return false;
                    }
                }
            }
            else {
                // Write all sectors of one track for side #1
                for (int sector_bytes = 0; sector_bytes < sectors_per_track * sector_size; sector_bytes++) {
                    if (!(bi_1 < side_image_bytes[1].end()) || !fout.write((char*)&(*bi_1++), 1)) {
                        cout << "Failed to write image bytes to file\n";
                        fout.close();
                        return false;
                    }
                }

            }
        }
    }


    if (mVerbose)
        cout << "Disc image created - " << fout.tellp() << " bytes written in total!\n";

    // Close disc image file
    fout.close();

   

    return true;
}