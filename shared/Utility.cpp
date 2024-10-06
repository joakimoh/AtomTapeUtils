#include "Utility.h"
#include "CommonTypes.h"
#include <iostream>
#include <sstream>
#include "Logging.h"
#include <filesystem>
#include "WaveSampleTypes.h"
#include <cmath>
#include <cstdint>


string Utility::roundedPD(string prefix, double d, string suffix)
{
    ostringstream s;
    if (d > 0)
        s << setw(4) << setprecision(2) << d << setw(1) << "s";
    else
        s << setw(5) << "-";

    return prefix + s.str()  + suffix;
}

void Utility::initbytes(Byte* bytes, Byte v, int n)
{
    for (int i = 0; i < n; i++) {
        bytes[i] = v;
    }
}

void Utility::initbytes(Bytes& bytes, Byte v, int n)
{
    for (int i = 0; i < n; i++) {
        bytes.push_back(v);
    }
}

void Utility::copybytes(Byte* from, Byte* to, int n)
{
    for (int i = 0; i < n; i++) {
        to[i] = from[i];
        to[i] = from[i];
    }
}

void Utility::copybytes(Byte * from, Bytes &to, int n)
{
    for (int i = 0; i < n; i++) {
        to.push_back(from[i]);
    }
}

uint32_t Utility::bytes2uint(Byte* bytes, int n, bool littleEndian)
{
    uint32_t u = 0;
    for (int i = 0; i < n && i < 4; i++) {
        if (littleEndian)
            u = (u << 8) & 0xffffff00 |  *(bytes + n - 1 - i);
        else
            u = (u << 8) & 0xffffff00 | + *(bytes+i);
    }
    return u;

}

void Utility::uint2bytes(uint32_t u, Byte* bytes, int n, bool littleEndian)
{
    for (int i = 0; i < n && i < 4; i++) {
        if (littleEndian)
            bytes[i] = (u >> (i*8)) & 0xff;
        else
            bytes[n - 1 - i] = (u >> (i*8)) & 0xff;
    }

}

string Utility::crDefaultOutFileName(string filePath, string fileExt)
{
    filesystem::path file_path = filePath;
    filesystem::path dir_path = file_path.parent_path();
    filesystem::path file_base = file_path.stem();

    string file_ext = file_path.extension().string();

    filesystem::path out_file_name;
    if (fileExt.size() == 0)
        out_file_name = file_base.string();
    else
        out_file_name = file_base.string() + "." + fileExt;

    filesystem::path file_out_path = dir_path / out_file_name;

    return file_out_path.string();
}
string Utility::crDefaultOutFileName(string filePath)
{
    filesystem::path file_path = filePath;
    filesystem::path dir_path = file_path.parent_path();
    filesystem::path file_base = file_path.stem();

    string file_ext = file_path.extension().string();

    filesystem::path out_file_name = file_base.string() + "_out" + file_ext;

    filesystem::path file_out_path = dir_path / out_file_name;

    return file_out_path.string();
}
string Utility::crEncodedFileNamefromDir(string dirPath, TapeFile tapeFile, string fileExt)
{
    filesystem::path dir_path = dirPath;
    filesystem::path file_base = tapeFile.validFileName;

    string suffix = "";
    string file_ext;
    if (fileExt.size() == 0)
        file_ext = "";
    else
        file_ext = "." + fileExt;

    if (tapeFile.corrupted)
        suffix = "_corrupted";

    if (tapeFile.complete)
        suffix += file_ext;
    else
        suffix += "_incomplete_" + to_string(tapeFile.firstBlock) + "_" + to_string(tapeFile.lastBlock) + file_ext;

    filesystem::path output_file_name = dir_path / file_base;
    output_file_name += suffix;

    return output_file_name.string();;
}

/*
* Parse time on format mm:ss.ss
*/
double Utility::decodeTime(string time)
{
    stringstream ts(time);
    double hour, min, sec;
    char c;

    ts >> hour >> c >> min >> c >> sec;

    return (hour * 3600 + min * 60 + sec);

}

string Utility::encodeTime(double t) {
    int t_h = (int)trunc(t / 3600);
    int t_m = (int)trunc((t - t_h * 3600) / 60);
    double t_s = t - t_h * 3600 - t_m * 60;
    int t_si = (int) t_s;
    int t_sf = (int) round((t_s - t_si) * 10000);
    if (t_sf == 10000) {
        t_sf = 0;
        t_si++;
        if (t_si == 60) {
            t_si = 0;
            t_m++;
            if (t_m == 60) {
                t_h++;
            }
        }
    }
    stringstream s;
    s <<
        right << setw(2) << setfill('0') << t_h << ":" <<
        right << setw(2) << setfill('0') << t_m << ":" <<
        right << setw(2) << setfill('0') << t_si << "." <<
        right << setw(4) << setfill('0') << t_sf << " " <<
        right << setfill(' ') << "(" << setprecision(4) << setw(5) << t << "s)";
    return s.str();
}

char Utility::digitToHex(int d)
{
    if (d >= 0 && d <= 9)
        return (char)(d + int('0'));
    else if (d >= 10 && d <= 15)
        return (char)(d - 10 + int('a'));
    else
        return 'X';
}

void Utility::logData(int address, Byte* data, int sz)
{
    return logData(&cout, address, data, sz);
}
void Utility::logData(ostream * fout, int address, Byte* data, int sz)
{

    vector<Byte> bytes;
    for (int i = 0; i < sz; i++)
        bytes.push_back(*(data+i));
    BytesIter bi = bytes.begin();

    Utility::logData(fout, address, bi, sz);

}
void Utility::logData(int address, BytesIter& data_iter, int data_sz) {

    return logData(&cout, address, data_iter, data_sz);
}
void Utility::logData(ostream *fout, int address, BytesIter &data_iter, int data_sz) {

    BytesIter di = data_iter;
    BytesIter di_16 = data_iter;
    int a = address;
    bool new_line = true;
    int line_sz;

    for (int i = 0; i < data_sz; i++) {
        if (new_line) {   
            new_line = false;
            line_sz = (i > data_sz - 16 ? data_sz - i : 16);
            *fout << "\n" << hex << setw(4) << setfill('0') << (int) a << " ";
        }
        *fout << hex << setw(2) << setfill('0') << (int) *di++ << " ";

        if (i % 16 == line_sz - 1) {
            new_line = true;
            for (int k = 0; k < 16 - line_sz; k++)
                *fout << "   ";
            *fout << " ";
            for (int j = i; j < i + line_sz; j++) {
                if (*di_16 >= 0x20 && *di_16 <= 0x7e)
                    *fout << (char)*di_16;
                else
                    *fout << ".";
                di_16++;
            }
            a += line_sz;
            di_16 = di;
        }

    }

    *fout << "\n";
}