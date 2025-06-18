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
bool MMBCodec::encode(string &discDir, string &MMBFile)
{
	vector<string> SSD_files;

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
	cout << "Creating MMB file '" << fout_name << "'\n";

	// Write MMB signature
	fout.write("\x00\x01\x02\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);

	// Write disc titles
	int n_titles = 0;
	for (const auto& entry : fs::directory_iterator(discDir)) {
		// Converting the path to const char * in the
		// subsequent lines
		fs::path file_path = entry.path();
		if (fs::is_regular_file(fs::status(file_path.c_str())) && file_path.extension() == ".ssd") {
			if (n_titles < 511) {
				SSD_files.push_back(file_path.string());
				string disc_title = file_path.stem().string();
				if (disc_title.size() > 12)
					disc_title = disc_title.substr(0, 12);
				else if (disc_title.size() < 12)
					disc_title.insert(disc_title.size(), 12 - disc_title.size(), '\0');
				cout << "Adding title '" << disc_title << "' (" << disc_title.size() << ")\n";
				disc_title = disc_title + "\x00\x00\x00\x0f";
				fout.write(disc_title.c_str(), 16);
			}
			else {
				cout << "too many SSD files (max 511 is allowed) => skipping SSD file '" << file_path.filename() << "'\n";
			}
		};
		n_titles++;
	}
	// Fill non-used disc title space wth zeros
	for (int t = 0; t < (511 - SSD_files.size()) * 16; t++)
		fout.put(0x0);

	// Add SSD file content
	for (int f = 0; f < SSD_files.size(); f++) {
		ifstream fin(SSD_files[f], ios::in | ios::binary | ios::ate);
		fin.seekg(0);
		char c;
		while (fin.read(&c, 1))
			fout.put(c);
		fin.close();
	}

	fout.close();

	return true;
}

// Decode an MMB file and extract the single-density Acorn DFS 200K SSD disc images it contains
bool MMBCodec::decode(string& MMBFileName, string& discDir)
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

	cout << "Decode file '" << MMBFileName << "' of size " << dec << file_sz << "\n";

	// Read header
	char* ref_header = "\x00\x01\x02\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
	char header[16];
	if (!MMB_file.read(&header[0], 16) || strncmp(header, ref_header,16) != 0) {
		cout << "File '" << MMBFileName << "' is not a valid MMB file!\n";
		MMB_file.close();
		return false;
	}

	cout << "MMB header was correct...\n";

	// Read disc titles
	vector<string> titles;
	for (int i = 0; i < 511; i++) {
		char title_a[16];
		if (!MMB_file.read(&title_a[0], 16)) {
			cout << "Premature ending of MMB file '" << MMBFileName << "''s disc titles\n";
			MMB_file.close();
			return false;
		}
		string title = title_a;
		while (title.size()>0 && title[title.size()-1] == ' ')
			title = title.substr(0, title.size() - 1);
		string title_suffix;
		for (int j = 12; j < 16; title_suffix += title_a[j++]);
		if (title != "" && title_suffix != "\x00\x00\x00\x0f"s) {
			cout << "Invalid disc title suffix for title '" << title << "' for MMB file '" << MMBFileName << "'\n";
			MMB_file.close();
			return false;
		}
		if (title != "") {
			titles.push_back(title);
		}
	}

	// Read SSD disc images
	for (int i = 0; i < titles.size(); i++) {
		string &title = titles[i];
		cout << "disc title '" << title << "' (" << title.size() << ")\n";

		// Create SSD file
		fs::path dir_path = discDir;
		fs::path file_path = dir_path / (title + ".ssd");
		string fout_name = file_path.string();
		ofstream fout(fout_name, ios::out | ios::binary | ios::ate);
		if (!fout) {
			cout << "can't write to file " << fout_name << "\n";
			return false;
		}
		cout << "Creating SSD file '" << fout_name << "'\n";

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
	
	MMB_file.close();

	return true;
}