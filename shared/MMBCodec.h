#ifndef MMB_CODEC_H
#define MMB_CODEC_H

#include <string>
#include "Logging.h"


using namespace std;

class MMBCodec
{
	Logging mLogging;

	enum TitleAccessEnum {MMB_LOCKED = 0x00, MMB_RW = 0x0f, MMB_UNFORMATTED = 0xf0, MMB_INVALID = 0xff};
#define _TITLE_ACCESS(x) (x==MMB_LOCKED?"Locked":(x==MMB_RW?"R/W":(x==MMB_UNFORMATTED?"Unformatted":(x==MMB_INVALID?"Invalid":"???"))))

public:

	MMBCodec::MMBCodec(Logging logging) : mLogging(logging) {};

	// Encode single-density Acorn DFS 200K SSD disc images as an MMB file
	bool encode(string& discDir, string& MMBFileName);

	// Decode an MMB file and extract the single-density Acorn DFS 200K SSD disc images it contains
	bool decode(string& MMBFileName, string& discDir, bool catOnly);


};

#endif