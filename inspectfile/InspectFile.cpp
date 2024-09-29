
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>
#include <cstdint>
#include "ArgParser.h"


using namespace std;
using namespace std::filesystem;

int main(int argc, const char* argv[])
{

    ArgParser arg_parser = ArgParser(argc, argv);

    if (arg_parser.failed())
        return -1;
        
    ostream* fout_p = &cout;
    bool write_to_file = false;
    if (arg_parser.dstFileName != "") {
        write_to_file = true;
        cout << "Output file name = " << arg_parser.dstFileName << "\n";
        fout_p = new ofstream(arg_parser.dstFileName);
        if (!*fout_p) {
            cout << "can't write to file " << arg_parser.dstFileName << "\n";
            return (-1);
        }
    }


    ifstream fin(arg_parser.srcFileName, ios::in | ios::binary | ios::ate);

    if (!fin) {
        *fout_p << "couldn't open file " << arg_parser.srcFileName << "\n";
        return (-1);
    }

    // Create vector the same size as the file
    auto fin_sz = fin.tellg();
    vector<uint8_t> data(fin_sz);


    // Fill the vector with the file content
    uint8_t* data_p = &data.front();
    fin.seekg(0);
    fin.read((char*)data_p, (streamsize)fin_sz);
    fin.close();

    vector<uint8_t>::iterator data_iter = data.begin();


    int count = 0;
    int pos = 0;
    while (data_iter < data.end() && count < 1024) {
        vector<uint8_t>::iterator line_iter = data_iter;
        char s[32];
        sprintf(s, "%4.4x ", pos);
        *fout_p << s;
        int i;
        for (i = 0; i < 16 && line_iter < data.end(); i++) {
            char s[32];
            sprintf(s, "%2.2x ", (int)*line_iter++);
            *fout_p << s;
        }
        for (int j = i; j < 16; j++) {
            char s[32];
            sprintf(s, "   ");
            *fout_p << s;
        }
        line_iter = data_iter;
        for (int i = 0; i < 16 && line_iter < data.end(); i++) {
            char c = *line_iter++;
            char s[32];
            if (c >= 0x20 && c <= 0x7e)
                sprintf(s, "%2c ", c);
            else
                sprintf(s, "  .");
            *fout_p << s;
            data_iter++;
        }
        *fout_p << "\n";

        count++;

        pos += 16;
    }
    
    if (write_to_file) {
        ((ofstream*)fout_p)->close();
        delete fout_p;
    }

    return 0;





}


