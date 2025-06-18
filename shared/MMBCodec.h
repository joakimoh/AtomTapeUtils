#ifndef MMB_CODEC_H
#define MMB_CODEC_H

#include <string>
#include "Logging.h"


using namespace std;

class MMBCodec
{
	Logging mLogging;

public:

	MMBCodec::MMBCodec(Logging logging) : mLogging(logging) {};

	// Encode single-density Acorn DFS 200K SSD disc images as an MMB file
	bool encode(string& discDir, string& MMBFileName);

	// Decode an MMB file and extract the single-density Acorn DFS 200K SSD disc images it contains
	bool decode(string& MMBFileName, string& discDir);


};

#endif