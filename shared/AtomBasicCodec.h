#pragma once

#include "BlockTypes.h"

class AtomBasicCodec
{

private:
	TAPFile mTapFile;
	bool mVerbose = false;

public:
	AtomBasicCodec(TAPFile &tapFile, bool verbose);
	AtomBasicCodec(bool verbose);

	/*
	 * Encode TAP File structure as Atom Basic program file
	 */
	bool encode(string & filePath);
	
	/*
	 * Decode Atom Basic program file as TAP File Structure
	 */
	bool decode(string &programFileName);

	bool getTAPFile(TAPFile& tapFile);

	bool setTAPFile(TAPFile& tapFile);
};

