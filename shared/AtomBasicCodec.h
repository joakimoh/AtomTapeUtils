#pragma once

#include "FileBlock.h"
#include <map>
#include "Logging.h"


enum TokeniseState {
	PARSING_GOTO_LINE_NO,
	NO_TOKENISATION,
	PARSING_FN_PROC_NAME,
	TOKENISATION,
	START_OF_LINE,
	PARSING_STRING
};

enum TokeniseInfo {
	CONDITIONAL_TOKENISATION = 0x1,
	MIDDLE_OF_STATEMENT = 0x2,
	START_OF_STATEMENT = 0x4,
	FN_OR_PROC_NAME = 0x8,
	GOTO_LINE = 0x10,
	STOP_TOKENISE = 0x20,
	ADD_40_FOR_START_OF_STATEMENT = 0x40
};

class TokenEntry
	// Bit 0 - Don't tokenise if followed by an alphabetic character.
	// Bit 1 - Go into "middle of Statement" mode.
	// Bit 2 - Go into "Start of Statement" mode.
	// Bit 3 - FN/PROC keyword - don't tokenise the name of the subroutine
	// Bit 4 - Tokenise the line number
	// Bit 5 - Don't tokenise rest of line (REM, DATA, etc...)
	// Bit 6 - Add 0x40 to token id if at the start of a statement/hex number
	//
{
public:
	string fullT = "";
	string shortT = "";
	Byte id1 = 0x0;
	Byte tokeniseInfo = 0x0;

	
	
};




class AtomBasicCodec
{

private:
	Logging mDebugInfo;
	TargetMachine mTargetMachine = ACORN_ATOM;

	
	vector<TokenEntry> mBBMTokens
	{
		{"ABS",			"ABS",		0x94, 0x00},
		{"ACS",			"ACS",		0x95, 0x00},
		{"ADVAL",		"AD.",		0x96, 0x00},
		{"AND",			"A.",		0x80, 0x00},
		{"ASC",			"ASC",		0x97, 0x00},
		{"ASN",			"ASN",		0x98, 0x00},
		{"ATN",			"ATN",		0x99, 0x00},
		{"AUTO",		"AU.",		0xC6, 0x10},
		{"BGET",		"B.",		0x9A, 0x01},
		{"BPUT",		"BP.",		0xD5, 0x03},
		{"CALL",		"CA.",		0xD6, 0x02},
		{"CHAIN",		"CH.",		0xD7, 0x02},
		{"CHR$",		"CHR.",		0xBD, 0x00},
		{"CLEAR",		"CL.",		0xD8, 0x01},
		{"CLG",			"CLG",		0xDA, 0x01},
		{"CLOSE",		"CLO.",		0xD9, 0x03},
		{"CLS",			"CLS",		0xDB, 0x01},
		{"COLOUR",		"C.",		0xFB, 0x02},
		{"COS",			"COS",		0x9B, 0x00},
		{"COUNT",		"COU.",		0x9C, 0x01},
		{"DATA",		"D.",		0xDC, 0x20},
		{"DEF",			"DEF",		0xDD, 0x00},
		{"DEG",			"DEG",		0x9D, 0x00},
		{"DELETE",		"DEL.",		0xC7, 0x10},
		{"DIM",			"DIM",		0xDE, 0x02},
		{"DIV",			"DIV",		0x81, 0x00},
		{"DRAW",		"DR.",		0xDF, 0x02},
		{"ELSE",		"EL.",		0x8B, 0x14},
		{"END",			"END",		0xE0, 0x01},
		{"ENDPROC",		"E.",		0xE1, 0x01},
		{"ENVELOPE",	"ENV.",		0xE2, 0x02},
		{"EOF",			"EOF",		0xC5, 0x01},
		{"EOR",			"EOR",		0x82, 0x00},
		{"ERL",			"ERL",		0x9E, 0x01},
		{"ERR",			"ERR",		0x9F, 0x01},
		{"ERROR",		"ERR.",		0x85, 0x04},
		{"EVAL",		"EV.",		0xA0, 0x00},
		{"EXP",			"EXP",		0xA1, 0x00},
		{"EXT",			"EXT",		0xA2, 0x01},
		{"FALSE",		"FA.",		0xA3, 0x01},
		{"FN",			"FN",		0xA4, 0x08},
		{"FOR",			"F.",		0xE3, 0x02},
		{"GCOL",		"GC.",		0xE6, 0x02},
		{"GET",			"GET",		0xA5, 0x00},
		{"GET$",		"GE.",		0xBE, 0x00},
		{"GOSUB",		"GOS.",		0xE4, 0x12},
		{"GOTO",		"G.",		0xE5, 0x12},
		{"HIMEM",		"H.",		0x93, 0x43},	//right,left=> add 0x40 to id
		{"IF",			"IF",		0xE7, 0x02},
		{"INKEY",		"INKEY",	0xA6, 0x00},
		{"INKEY$",		"INK.",		0xBF, 0x00},
		{"INPUT",		"I.",		0xE8, 0x02},
		{"INSTR(",		"INS.",		0xA7, 0x00},
		{"INT",			"INT",		0xA8, 0x00},
		{"LEFT$(",		"LE.",		0xC0, 0x00},
		{"LEN",			"LEN",		0xA9, 0x00},
		{"LET",			"LET",		0xE9, 0x04},
		{"LINE",		"LIN.",		0x86, 0x00},
		{"LIST",		"L.",		0xC9, 0x10},
		{"LN",			"LN",		0xAA, 0x00},
		{"LOAD",		"LO.",		0xC8, 0x02},
		{"LOCAL",		"LOC.",		0xEA, 0x02},
		{"LOG",			"LOG",		0xAB, 0x00},
		{"LOMEM",		"LOM.",		0x92, 0x43},	//right,left=> add 0x40 to id
		{"MID$(",		"M.",		0xC1, 0x00},
		{"MOD",			"MOD",		0x83, 0x00},
		{"MODE",		"MO.",		0xEB, 0x02},
		{"MOVE",		"MOV.",		0xEC, 0x02},
		{"NEW",			"NEW",		0xCA, 0x01},
		{"NEXT",		"N.",		0xED, 0x02},
		{"NOT",			"NOT",		0xAC, 0x00},
		{"OFF",			"OFF",		0x87, 0x00},
		{"OLD",			"O.",		0xCB, 0x01},
		{"ON",			"ON",		0xEE, 0x02},
		{"OPENIN",		"OP.",		0x8E, 0x00},
		{"OPENOUT",		"OPENO.",	0xAE, 0x00},
		{"OPENUP",		"OPENUP",	0xAD, 0x00},
		//{"OPT",		"OPT",		 -1,  0x00}, // This keyword is not tokenized!
		{"OR",			"OR",		0x84, 0x00},
		{"OSCLI",		"OSC.",		0xFF, 0x00},
		{"PAGE",		"PA.",		0x90, 0x43},	//right,left=> add 0x40 to id
		{"PI",			"PI",		0xAF, 0x01},
		{"PLOT",		"PL.",		0xF0, 0x02},
		{"POINT(",		"PO.",		0xB0, 0x00},
		{"POS",			"POS",		0xB1, 0x01},
		{"PRINT",		"P.",		0xF1, 0x02},
		{"PROC",		"PRO.",		0xF2, 0x0a},
		{"PTR",			"PT.",		0x8F, 0x43},	//right,left=> add 0x40 to id
		{"RAD",			"RAD",		0xB2, 0x00},
		{"READ",		"REA.",		0xF3, 0x02},
		{"REM",			"REM",		0xF4, 0x20},
		{"RENUMBER",	"REN.",		0xCC, 0x10},
		{"REPEAT",		"REP.",		0xF5, 0x00},
		{"REPORT",		"REPO.",	0xF6, 0x01},
		{"RESTORE",		"RES.",		0xF7, 0x12},
		{"RETURN",		"R.",		0xF8, 0x01},
		{"RIGHT$(",		"RI.",		0xC2, 0x00},
		{"RND",			"RND",		0xB3, 0x01},
		{"RUN",			"RUN",		0xF9, 0x01},
		{"SAVE",		"SA.",		0xCD, 0x02},
		{"SGN",			"SGN",		0xB4, 0x00},
		{"SIN",			"SIN",		0xB5, 0x00},
		{"SOUND",		"SO.",		0xD4, 0x02},
		{"SPC",			"SPC",		0x89, 0x00},
		{"SQR",			"SQR",		0xB6, 0x00},
		{"STEP",		"S.",		0x88, 0x00},
		{"STOP",		"STO.",		0xFA, 0x01},
		{"STR$",		"STR.",		0xC3, 0x00},
		{"STRING$(",	"STRI.",	0xC4, 0x00},
		{"TAB(",		"TAB(",		0x8A, 0x00},
		{"TAN",			"T.",		0xB7, 0x00},
		{"THEN",		"TH.",		0x8C, 0x14},
		{"TIME",		"TI.",		0x91, 0x43},	//right,left=> add 0x40 to id
		{"TO",			"TO",		0xB8, 0x00},
		{"TRACE",		"TR.",		0xFC, 0x12},
		{"TRUE",		"TRUE",		0xB9, 0x01},
		{"UNTIL",		"U.",		0xFD, 0x02},
		{"USR",			"USR",		0xBA, 0x00},
		{"VAL",			"VAL",		0xBB, 0x00},
		{"VDU",			"V.",		0xEF, 0x02},
		{"VPOS",		"VP.",		0xBC, 0x01},
		{"WIDTH",		"W.",		0xFE, 0x02}
	};

	map<int, TokenEntry> mTokenDictId;
	map<string, TokenEntry> mTokenDictStr;

	// Encode/decode the line number (BBC Micro only)
	bool encodeLineNo(int lineNo, string &encodedBytes);
	bool decodeLineNo(string encodedBytes, int& lineNo);

	bool token2Int(string token, int& num);


public:

	AtomBasicCodec(Logging logging, TargetMachine targetMachine);


	// Decode tokenised (detokenise) data as BBC Basic source code
	bool detokenise(string& srcFile, string& dstFile);
	bool detokenise(TapeFile& tapFile, string &filePath);
	bool detokenise(string program, Bytes& tokenisedProgram, string& filePath, bool& faultyTermination);
	bool detokenise(string program, Bytes& tokenisedProgram, Bytes &sourceCode, bool& faultyTermination);


	 // Encode (tokenise) BBC Basic source code as tokenised data
	bool tokenise(string& srcFile, string& dstFile);
	bool tokenise(string &fullPathFileName, TapeFile& tapeFile);
	bool tokenise(string& fullPathFileName, Bytes& tokenisedData);
	bool tokenise(string program, Bytes& sourceCode, Bytes& tokenisedData);

private:

	bool readFileBytes(bool tokenisedBytes, string& srcFile, Bytes& data, string& program);
	bool writeFileBytes(bool tokenisedBytes, string program, Bytes& data, string& dstFile);

	bool getKeyWord(bool& fun_or_proc, bool startOfStatement, bool withinString, string& text, string &space, string &token, TokenEntry& entry);
	bool isDelimiter(char);
	bool nextToken(string& text, string& token);

	bool detokeniseAtom(string program, Bytes& tokenisedProgram, Bytes& sourceCode, bool& faultyTermination);
	bool detokeniseBBM(string program, Bytes& tokenisedProgram, Bytes& sourceCode, bool& faultyTermination);

	bool tokeniseLine(int lineNo, string &line, string  &tCode);

	bool matchAgainstKeyword(string& text, int pos, string& token, TokenEntry& entry);
};

