#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>

#include "TAPCodec.h"
#include "Utility.h"
#include "Debug.h"

using namespace std;

namespace fs = std::filesystem;


TAPCodec::TAPCodec()
{

}


TAPCodec::TAPCodec(TAPFile &tapFile): mTapFile(tapFile)
{

}



/*
 * Encode TAP File structure as TAP file
 */
bool TAPCodec::encode(string& filePath)
{

    if (mTapFile.blocks.empty())
        return false;


    ofstream fout(filePath, ios::out | ios::binary | ios::ate);
    if (!fout) {
        cout << "can't write to TAP file " << filePath << "\n";
        return false;
    }

    ATMBlockIter ATM_block_iter = mTapFile.blocks.begin();


    while (ATM_block_iter < mTapFile.blocks.end()) {

        //
        // Format of Acorn Atom BASIC program in memory:
        // {<cr> <linehi> <linelo> <text>} <cr> <ff>
        //
        BytesIter bi = ATM_block_iter->data.begin();
        int exec_adr = ATM_block_iter->hdr.execAdrHigh * 256 + ATM_block_iter->hdr.execAdrLow;
        int load_adr = ATM_block_iter->hdr.loadAdrHigh * 256 + ATM_block_iter->hdr.loadAdrLow;
        int data_len = ATM_block_iter->hdr.lenHigh * 256 + ATM_block_iter->hdr.lenLow;
        string name = ATM_block_iter->hdr.name;

        while (bi < ATM_block_iter->data.end()) {
            fout.write((char*)&(*bi++), 1);

        }
        ATM_block_iter++;
    }

    fout.close();

    DBG_PRINT(DBG, "TAP file '%s' created from %ld blocks...\n", filePath.c_str(), mTapFile.blocks.size());

    return true;

}

/*
 * Decode TAP file as TAP File structure
 */
bool TAPCodec::decode(string& tapFileName)
{
    return true;
}

/*
 * Get the codec's TAP file
 */
bool TAPCodec::getTAPFile(TAPFile& tapFile)
{
    tapFile = mTapFile;

    return true;
}



/*
 * Reinitialise codec with a new TAP file
 */
bool TAPCodec::setTAPFile(TAPFile& tapFile)
{
    mTapFile = tapFile;

    return true;

}
