#include "FileDecoder.h"
#include "AtomBlockDecoder.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include "../shared/Debug.h"
#include "../shared/AtomBasicCodec.h"
#include "../shared/UEFCodec.h"
#include "../shared/TAPCodec.h"
#include "../shared/Utility.h"

namespace fs = std::filesystem;

using namespace std;




FileDecoder::FileDecoder(
    ArgParser& argParser
) : mArgParser(argParser)
{
    mTracing = argParser.tracing;
    mVerbose = argParser.verbose;
}


string FileDecoder::timeToStr(double t) {
    char t_str[64];
    int t_h = (int)trunc(t / 3600);
    int t_m = (int)trunc((t - t_h) / 60);
    double t_s = t - t_h * 3600 - t_m * 60;
    sprintf(t_str, "%2d:%2d:%9.6f (%12f)", t_h, t_m, t_s, t);
    return string(t_str);
}
