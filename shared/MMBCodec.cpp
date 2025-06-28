#include "MMBCodec.h"
#include <iostream>
#include <fstream>
#include <filesystem>


namespace fs = std::filesystem;


//
// Create an MMB image
// 
// From https://github.com/hoglet67/MMFS/wiki/MMB-File-Format:
// An MMB file stores between 1 and 511 200KB Acorn DFS disk images.
// Every MMB File has an 8192 - byte header which with a signature at the start followed by 511 spaces for the disk titles
// The first 16 bytes of the file contains a signature which is 00 01 02 03 00 00 ..
// Each subsequent 16 Bytes stores a disk title(12 characters / bytes) then padded with 00 00 00 0F.
// The rest of the file is a collection of 200K Byte SSD images in disk order.
// 
// An MMB file comes in the 'basic' format and the 'extended' format.
// 
// The 'basic' format can store up to 511 200KB Acorn DFS disk images.
//		<MMB> ::= <header><disk image>...<disk image>
//		<header> ::= <signature> <disc title> ... <disc title>
//		<signature> ::= aa bb cc dd AA BB CC DD 00 00 00 00 00 00 00 00
//		<disc title> ::= <12 char title right-padded with zeroes> 00 00 00 <status>
//		<status> ::= 00 (locked) | 0f (R/W) | f0 (unformatted) | ff (invalid)
// 
// where AAaa BBbb CCcc DDdd are boot time images
// 
// The 'extended' format is simply a sequence of MMB chunks with a slighlty different MMB signature
// 
//		<extended MMB> ::== <basic MMB> ... <basic MMB>
//		<basic MMB> ::= <extended header> <disc image> ... <disk image>
//		<extended header> ::= <extended signature> <disc title> ... <disc title>
//		<extended signature> ::= aa bb cc dd AA BB CC DD NN 00 00 00 00 00 00 00
//
// where NN is the no of chunks,n , encoded as '0xa0 | n'
//
bool MMBCodec::encode(string &discDir, string &MMBFile)
{
	vector<vector<string>> MMB_files;

	// This structure would distinguish a file from a
	// directory
	struct stat sb;

	if (stat(discDir.c_str(), &sb) != 0) {
		cout << "Directory '" << discDir << "' doesn't exist!\n";
		return false;
	}

	string fout_name = MMBFile;
	ofstream fout(fout_name, ios::out | ios::binary | ios::ate);
	if (!fout) {
		cout << "can't write to file " << fout_name << "\n";
		return false;
	}

	if (mLogging.verbose)
		cout << "Creating MMB file '" << fout_name << "'\n";

	// Count the no of SSD files
	int n_MMB_chunks = 0;
	int n_SSD_files = 0;
	vector<string> SSD_files;
	for (const auto& entry : fs::directory_iterator(discDir)) {

		fs::path file_path = entry.path();
		if (fs::is_regular_file(fs::status(file_path.c_str())) && file_path.extension() == ".ssd") {

			if (n_SSD_files < 511 * 16) {

				SSD_files.push_back(file_path.string());

				if (n_SSD_files % 511 == 0)
					n_MMB_chunks++;
				n_SSD_files++;

				if (n_SSD_files % 511 == 0) {
					MMB_files.push_back(SSD_files);
					SSD_files.clear();
				}
			}
		}
	}
	if (n_SSD_files > 0 && n_SSD_files % 511 != 0)
		MMB_files.push_back(SSD_files);

	if (n_MMB_chunks == 0) {
		cout << "No SSD files found => quitting...\n";
	}
	else if (n_MMB_chunks > 15) {
		cout << "Too many SSD files (" << dec << n_SSD_files << ") found. Only " << 16 * 511 <<
			" files can fit in the max no of MMB chunks (15) => will not be able to include all SSD files!\n";
		n_MMB_chunks = 15;
	}

	if (mLogging.verbose) {
		if (n_MMB_chunks > 1)
			cout << dec << n_SSD_files << " SSD files found. They will be put into an extended MMB file with " << n_MMB_chunks << " chunks.\n";
		else
			cout << dec << n_SSD_files << " SSD files found. They will be put into one standard MMB file\n";
	}

	// Write MMB chunks
	int n_titles = 0;
	const char *empty_title = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
	for (int m = 0; m < MMB_files.size(); m++) {

		// Write MMB chunk header

		// Write MMB signature (with status byte 0x0f <=> R/W and boot time images 0, 1, 2 & 3)
		vector<uint8_t> signature(16, 0x0);
		uint16_t boot_image_adr[4];
		boot_image_adr[0] = 0x0;
		boot_image_adr[1] = 0x1;
		boot_image_adr[2] = 0x2;
		boot_image_adr[3] = 0x3;
		for (int i = 0; i < 4; i++) {
			signature[i] = boot_image_adr[i] & 0xf;
			signature[i + 4] = boot_image_adr[i] >> 8;
		}
		signature[8] = 0xa0 | (n_MMB_chunks-m-1);
		for (int i = 0; i < 16; i++)
			fout.write((char*)&signature[i], 1);

		// Write disc titles 
		vector<string>& SSD_files = MMB_files[m];

		for (int t = 0; t < 511; t++) {

			if (t < SSD_files.size()) {

				fs::path file_path = SSD_files[t];

				// Create disc title right-padded with zeroes
				string disc_title = file_path.stem().string();

				if (mLogging.verbose)
					cout << "Adding title '" << disc_title << "'\n";

				if (disc_title.size() > 12)
					disc_title = disc_title.substr(0, 12);
				else if (disc_title.size() < 12)
					disc_title.insert(disc_title.size(), 12 - disc_title.size(), '\0');
				disc_title = disc_title + "\x00\x00\x00\x0f"s;

				fout.write(disc_title.c_str(), 16);



			} else

				fout.write(empty_title, 16);
		}

		// Write disc images
		for (int t = 0; t < 511; t++) {
			ifstream fin(SSD_files[t], ios::in | ios::binary | ios::ate);
			fin.seekg(0);
			char c;
			while (fin.read(&c, 1))
				fout.put(c);
			fin.close();
		}


	}

	fout.close();

	return true;
}

// Decode an MMB file and extract the single-density Acorn DFS 200K SSD disc images it contains
bool MMBCodec::decode(string& MMBFileName, string& discDir, bool catOnly)
{
	ifstream MMB_file(MMBFileName, ios::in | ios::binary | ios::ate);

	if (!MMB_file) {
		cout << "couldn't open MMB File '" << MMBFileName << "'\n";
		return false;
	}

	// Get file size
	MMB_file.seekg(0, ios::end);
	streamsize file_sz = MMB_file.tellg();
	MMB_file.seekg(0);

	if (mLogging.verbose)
		cout << "Decode file '" << MMBFileName << "' of size " << dec << file_sz << "\n";

	// Read header
	char header[16];
	if (!MMB_file.read(&header[0], 16)) {
		cout << "File '" << MMBFileName << "' is not a valid MMB file!\n";
		MMB_file.close();
		return false;
	}

	uint16_t boot_image_0 = header[0] | (header[4] << 8);
	uint16_t boot_image_1 = header[1] | (header[5] << 8);
	uint16_t boot_image_2 = header[2] | (header[6] << 8);
	uint16_t boot_image_3 = header[3] | (header[7] << 8);
	uint8_t MMB_ext = header[8];
	uint8_t MMB_chunks = 1;
	if ((MMB_ext & 0xa0) == 0xa0) {
		MMB_chunks = (MMB_ext & 0xf) + 1;
	}

	uint16_t boot_image[4];
	for (int i = 0; i < 4; i++)
		boot_image[i] = header[i] | (header[i + 4] << 8);

	if (mLogging.verbose) {
		if (MMB_chunks > 1)
			cout << "MMB is an extended MMB with " << (int)MMB_chunks << " MMB chunks...\n";
		else
			cout << "MMB is a standard MMB\n";
		cout << "Boot time images are: ";
		for (int i = 0; i < 4; i++)
			cout << hex << "0x" << setw(4) << setfill('0') << boot_image[i] << " ";
		cout << "\n";
	}

	int null_title_index = 0;
	for (int chunk = 0; chunk < MMB_chunks; chunk++) {

		if (chunk > 0) {

			// Read header (not for the first chunk as that one was already read earlier)
			char header[16];
			if (!MMB_file.read(&header[0], 16)) {
				cout << "Header missing for the " << dec << chunk << " chunk!\n";
				MMB_file.close();
				return false;
			}
		}

		// Read disc titles
		vector<string> titles;
		vector<string> access;
		for (int i = 0; i < 511; i++) {
			char title_a[16];
			if (!MMB_file.read(&title_a[0], 16)) {
				cout << "Premature ending of MMB file '" << MMBFileName << "''s disc titles\n";
				MMB_file.close();
				return false;
			}
			string title = title_a;
			while (title.size() > 0 && title[title.size() - 1] == ' ')
				title = title.substr(0, title.size() - 1);
			string title_suffix;
			for (int j = 12; j < 16; title_suffix += title_a[j++]);

			// If the title is empty, then generate a unique title NULL_<no>
			if (title == "")
				title += "NULL_" + to_string(null_title_index++);

			// Store title and access status (locked, unformatted, invalid or ???)
			int p_sz = titles.size();
			titles.push_back(title);
			access.push_back(_TITLE_ACCESS(title_suffix[3]));

			if (mLogging.verbose) {
				if (MMB_chunks > 1)
					cout << "MMB #" << dec << chunk << ", disk #" << i << " title '" << title << "' with status " << _TITLE_ACCESS(title_suffix[3]) << "\n";
				else
					cout << "Disk #" << i << " title '" << title << "' with status " << _TITLE_ACCESS(title_suffix[3]) << "\n";
			}

		}

		// Read SSD disc images (if not only a catalogue shall be output)

		for (int i = 0; i < titles.size(); i++) {
			string& title = titles[i];
			string& acc = access[i];

			if (!catOnly) {

				// Create SSD file
				fs::path dir_path = discDir;
				fs::path file_path = dir_path / (title + ".ssd");
				string fout_name = file_path.string();
				ofstream fout(fout_name, ios::out | ios::binary | ios::ate);
				if (!fout) {
					cout << "can't write to file " << fout_name << "\n";
					return false;
				}

				if (mLogging.verbose) {
					cout << "Creating SSD file '" << fout_name << " for disc title '" << title << "' (" << title.size() << ")\n";
				}

				// Fill the SSD file with content from the MMB file
				for (int b = 0; b < 200 * 1024; b++) {
					char c;
					if (!MMB_file.read(&c, 1) || !fout.write(&c, 1)) {
						cout << "Failed to copy bytes from MMB file '" << MMBFileName << "' to SSD file '" << fout_name << "'!\n";
						MMB_file.close();
						fout.close();
						return false;
					}
				}
			}

			else {

				for (int b = 0; b < 200 * 1024; b++) {
					char c;
					if (!MMB_file.read(&c, 1)) {
						cout << "Premature ending of MMB file '" << MMBFileName << "''s disc titles\n";
						MMB_file.close();
						return false;
					}
				}
				if (MMB_chunks > 1)
					cout << setw(2) << dec << chunk << " " << setw(3) << i << " " << setw(12) << setfill(' ') << title << " " << acc << "\n";
				else
					cout << dec << setw(3) << i << " " << setw(12) << title << " " << acc << "\n";
			}

		}

		if (mLogging.verbose) {
			cout << "MMB file pos after read disc is: 0x" << hex << MMB_file.tellg() << "\n";
		}

	}


	
	MMB_file.close();

	return true;
}