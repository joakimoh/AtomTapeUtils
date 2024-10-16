#ifndef DISC_CODEC_H
#define DISC_CODEC_H

#include <string>
#include <map>
#include "CommonTypes.h"
#include "Logging.h"
#include <sstream>
#include "FileBlock.h"

//
// Codec for Acorn DFS discs (SSD)
//

#define MAX_NO_OF_TRACKS_PER_SIDE 80
#define NO_OF_SECTORS_PER_TRACK 10
#define SECTOR_SIZE 256
#define TRACK_SIZE (streampos) (NO_OF_SECTORS_PER_TRACK * NO_OF_RECORDS_PER_SECTOR * SECTOR_SIZE)

class Sector {
public:
	Byte record[SECTOR_SIZE];
};

class Track {
public:
	Sector sector[NO_OF_SECTORS_PER_TRACK];
};

class RawDiscSide {
public:
	Track track[MAX_NO_OF_TRACKS_PER_SIDE];
};

class RawDisc {
public:
	RawDiscSide side[2];
	int nSides = 1;
	int nTracks = 40;
};

class DiscFile {
public:
	string name;
	string dir;
	int loadAdr; // 18 bits
	int execAdr; // 18 bits
	int size; // 18 bits
	Bytes data;
	int startSector;
	bool locked = false;
};

class DiscDirectory {
public:
	string name; // One valid file name character - stored in the low 7 bits of sector 0 byte 15
	vector <DiscFile> file;
};

enum BootOption { NO_ACTION = 0, LOAD_ACTION = 1, RUN_ACTION = 2, EXEC_ACTION = 3 }; //  byte 6 b5b4 in sector 1
#define _BOOT_OPTION(x) ((x==BootOption::NO_ACTION?"No action":(x==BootOption::LOAD_ACTION?"*LOAD $.!BOOT":(x==\
BootOption::RUN_ACTION?"*RUN $.!BOOT":"*EXEC $.!BOOT"))))

// 
// One side (there could be up to two) of an Acorn DFS disc
// 
// There may be up to 31 files on the disc
//
class DiscSide {
public:
	string discTitle; // Space or null-padded ASCII chars - stored in sector 0 bytes 0 to 7 + sector 1 bytes 0 to 3.
	Byte cycleNo; // BDC-coded version of the disc (0-99) - stored in sector 1 byte 4
	BootOption bootOption; // 00: no action, 01: *LOAD $.!BOOT, 10: *RUN $.!BOOT, 11: *EXEC $.!BOOT - Stored in b5b4 of byte 6 in sector 1
	Word discSize = 0; // No of sectors - 10-bit: low 8 bits in sector 1 byte 7, high bits in the low two bits of sector 1 byte 6.
	Byte nFiles; // No of files - stored in sector 1 byte 5 as an offset (nFiles *8)
	Byte nSectors; // No of sectors - b7:b0 in sector 1 byte 7, b9b8 in b1b0 of sector 1 byte 6
	map<string, DiscDirectory> directory; // All the directories stored on the disc
	vector <DiscFile> files; // All the files stored on the disc
};

class Disc {
public:
	vector<DiscSide> side;
};

class DiscPos {
public:
	int side = 0;
	int track = 0;
	int sector = 0;
	int byte = 0;
	DiscPos() {}
	DiscPos(int s, int t, int sec, int b): side(s), track(t), sector(sec), byte(b) {}
	string toStr() {
		stringstream s;
		s << "Side " << side << ", Track " << track << ", Sector " << sector << ", Byte " << byte;
		return s.str();
	}
};

class DiscCodec {

private:
	bool mVerbose = false;
	ifstream *mFin_p = NULL;
	DiscPos mDiscPos;
	Disc *mDisc_p = NULL;

protected:
	bool move(int discSide, int trackNo, int sectorNo, int byteOffset);
	bool move2File(int discSide, int sectorNo);
	bool readBits(int highBitOffset, int nBits, uint64_t &bits);
	bool readByte(Byte& byte);
	bool readBytes(Byte* bytes, int n);
	bool readBytes(Bytes &bytes, int n);
	bool openDiscFile(string discPath, Disc& disc);
	bool closeDiscFile();
public:
	DiscCodec(Logging logging);
	bool read(string discPath, Disc & disc);
	bool write(string title, string discPath, vector<TapeFile> &tapeFiles);
};

#endif