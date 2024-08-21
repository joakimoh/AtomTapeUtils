#pragma once

#include "BlockTypes.h"
#include <map>


class TokenEntry
{
public:
	string fullT;
	string shortT;
	struct { int id1; int id2; };
};




class AtomBasicCodec
{

private:
	bool mVerbose = false;
	bool mBbcMicro = false;

	
	vector<TokenEntry> mBBMTokens
	{
		{"ABS",			"ABS",		{0x94,-1 }},
		{"ACS",			"ACS",		{0x95,-1 }},
		{"ADVAL",		"AD.",		{0x96,-1 }},
		{"AND",			"A.",		{0x80,-1 }},
		{"ASC",			"ASC",		{0x97,-1 }},
		{"ASN",			"ASN",		{0x98,-1 }},
		{"ATN",			"ATN",		{0x99,-1 }},
		{"AUTO",		"AU.",		{0xC6,-1 }},
		{"BGET",		"B.",		{0x9A,-1 }},
		{"BPUT",		"BP.",		{0xD5,-1 }},
		{"CALL",		"CA.",		{0xD6,-1 }},
		{"CHAIN",		"CH.",		{0xD7,-1 }},
		{"CHR$",		"CHR.",		{0xBD,-1 }},
		{"CLEAR",		"CL.",		{0xD8,-1 }},
		{"CLG",			"CLG",		{0xDA,-1 }},
		{"CLOSE",		"CLO.",		{0xD9,-1 }},
		{"CLS",			"CLS",		{0xDB,-1 }},
		{"COLOUR",		"C.",		{0xFB,-1 }},
		{"COS",			"COS",		{0x9B,-1 }},
		{"COUNT",		"COU.",		{0x9C,-1 }},
		{"DATA",		"D.",		{0xDC,-1 }},
		{"DEF",			"DEF",		{0xDD,-1 }},
		{"DEG",			"DEG",		{0x9D,-1 }},
		{"DELETE",		"DEL.",		{0xC7,-1 }},
		{"DIM",			"DIM",		{0xDE,-1 }},
		{"DIV",			"DIV",		{0x81,-1 }},
		{"DRAW",		"DR.",		{0xDF,-1 }},
		{"ELSE",		"EL.",		{0x8B,-1 }},
		{"END",			"END",		{0xE0,-1 }},
		{"ENDPROC",		"E.",		{0xE1,-1 }},
		{"ENVELOPE",	"ENV.",		{0xE2,-1 }},
		{"EOF",			"EOF",		{0xC5,-1 }},
		{"EOR",			"EOR",		{0x82,-1 }},
		{"ERL",			"ERL",		{0x9E,-1 }},
		{"ERR",			"ERR",		{0x9F,-1 }},
		{"ERROR",		"ERR.",		{0x85,-1 }},
		{"EVAL",		"EV.",		{0xA0,-1 }},
		{"EXP",			"EXP",		{0xA1,-1 }},
		{"EXT",			"EXT",		{0xA2,-1 }},
		{"FALSE",		"FA.",		{0xA3,-1 }},
		{"FN",			"FN",		{0xA4,-1 }},
		{"FOR",			"F.",		{0xE3,-1 }},
		{"GCOL",		"GC.",		{0xE6,-1 }},
		{"GET",			"GET",		{0xA5,-1 }},
		{"GET$",		"GE.",		{0xBE,-1 }},
		{"GOSUB",		"GOS.",		{0xE4,-1 }},
		{"GOTO",		"G.",		{0xE5,-1 }},
		{"HIMEM",		"H.",		{0x93,0xD3}},	//right,left
		{"IF",			"IF",		{0xE7,-1 }},
		{"INKEY",		"INKEY",	{0xA6,-1 }},
		{"INKEY$",		"INK.",		{0xBF,-1 }},
		{"INPUT",		"I.",		{0xE8,-1 }},
		{"INSTR(",		"INS.",		{0xA7,-1 }},
		{"INT",			"INT",		{0xA8,-1 }},
		{"LEFT$(",		"LE.",		{0xC0,-1 }},
		{"LEN",			"LEN",		{0xA9,-1 }},
		{"LET",			"LET",		{0xE9,-1 }},
		{"LINE",		"LIN.",		{0x86,-1 }},
		{"LIST",		"L.",		{0xC9,-1 }},
		{"LN",			"LN",		{0xAA,-1 }},
		{"LOAD",		"LO.",		{0xC8,-1 }},
		{"LOCAL",		"LOC.",		{0xEA,-1 }},
		{"LOG",			"LOG",		{0xAB,-1 }},
		{"LOMEM",		"LOM.",		{0x92,0xD2}},	//right,left
		{"MID$(",		"M.",		{0xC1,-1 }},
		{"MOD",			"MOD",		{0x83,-1 }},
		{"MODE",		"MO.",		{0xEB,-1 }},
		{"MOVE",		"MOV.",		{0xEC,-1 }},
		{"NEW",			"NEW",		{0xCA,-1 }},
		{"NEXT",		"N.",		{0xED,-1 }},
		{"NOT",			"NOT",		{0xAC,-1 }},
		{"OFF",			"OFF",		{0x87,-1 }},
		{"OLD",			"O.",		{0xCB,-1 }},
		{"ON",			"ON",		{0xEE,-1 }},
		{"OPENIN",		"OP.",		{0x8E,-1 }},
		{"OPENOUT",		"OPENO.",	{0xAE,-1 }},
		{"OPENUP",		"OPENUP",	{0xAD,-1 }},
		{"OPT",			"OPT",		{-1,-1 }},
		{"OR",			"OR",		{0x84,-1 }},
		{"OSCLI",		"OSC.",		{0xFF,-1 }},
		{"PAGE",		"PA.",		{0x90,0xD0}},	//right,left
		{"PI",			"PI",		{0xAF,-1 }},
		{"PLOT",		"PL.",		{0xF0,-1 }},
		{"POINT(",		"PO.",		{0xB0,-1 }},
		{"POS",			"POS",		{0xB1,-1 }},
		{"PRINT",		"P.",		{0xF1,-1 }},
		{"PROC",		"PRO.",		{0xF2,-1 }},
		{"PTR",			"PT.",		{0x8F,0xCF}},	//right,left
		{"RAD",			"RAD",		{0xB2,-1 }},
		{"READ",		"REA.",		{0xF3,-1 }},
		{"REM",			"REM",		{0xF4,-1 }},
		{"RENUMBER",	"REN.",		{0xCC,-1 }},
		{"REPEAT",		"REP.",		{0xF5,-1 }},
		{"REPORT",		"REPO.",	{0xF6,-1 }},
		{"RESTORE",		"RES.",		{0xF7,-1 }},
		{"RETURN",		"R.",		{0xF8,-1 }},
		{"RIGHT$(",		"RI.",		{0xC2,-1 }},
		{"RND",			"RND",		{0xB3,-1 }},
		{"RUN",			"RUN",		{0xF9,-1 }},
		{"SAVE",		"SA.",		{0xCD,-1 }},
		{"SGN",			"SGN",		{0xB4,-1 }},
		{"SIN",			"SIN",		{0xB5,-1 }},
		{"SOUND",		"SO.",		{0xD4,-1 }},
		{"SPC",			"SPC",		{0x89,-1 }},
		{"SQR",			"SQR",		{0xB6,-1 }},
		{"STEP",		"S.",		{0x88,-1 }},
		{"STOP",		"STO.",		{0xFA,-1 }},
		{"STR$",		"STR.",		{0xC3,-1 }},
		{"STRING$(",	"STRI.",	{0xC4,-1 }},
		{"TAB(",		"TAB(",		{0x8A,-1 }},
		{"TAN",			"T.",		{0xB7,-1 }},
		{"THEN",		"TH.",		{0x8C,-1 }},
		{"TIME",		"TI.",		{0x91,0xD1}},	//right,left
		{"TO",			"TO",		{0xB8,-1 }},
		{"TRACE",		"TR.",		{0xFC,-1 }},
		{"TRUE",		"TRUE",		{0xB9,-1 }},
		{"UNTIL",		"U.",		{0xFD,-1 }},
		{"USR",			"USR",		{0xBA,-1 }},
		{"VAL",			"VAL",		{0xBB,-1 }},
		{"VDU",			"V.",		{0xEF,-1 }},
		{"VPOS",		"VP.",		{0xBC,-1 }},
		{"WIDTH",		"W.",		{0xFE,-1 }}
	};

	map<int, TokenEntry> mTokenDictId;
	map<string, TokenEntry> mTokenDictStr;


public:
	AtomBasicCodec(bool verbose, bool bbcMicro);

	/*
	 * Encode TAP File structure as Atom Basic program file
	 */
	bool encode(TapeFile& tapFile, string &filePath);

	
	
	/*
	 * Decode Atom Basic program file as TAP File Structure
	 */
	bool decode(string &fullPathFileName, TapeFile& tapFile);

public:

	bool AtomBasicCodec::getKeyWord(bool startOfStatement, bool withinString, string& text, string &space, string &token, TokenEntry& entry);
	bool isDelimiter(char);
	bool nextToken(string& text, string& token);

	bool encodeAtom(TapeFile& tapeFile, string& filePath, ofstream& fout);
	bool encodeBBM(TapeFile& tapeFile, string& filePath, ofstream& fout);

	bool decodeAtom(Bytes &data, TapeFile& tapeFile, string file_name, string block_name);
	bool decodeBBM(Bytes &data, TapeFile& tapeFile, string file_name,  string block_name);

	bool tokenizeLine(string &line, string  &tCode);
};

