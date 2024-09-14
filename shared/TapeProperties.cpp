#include "TapeProperties.h"
#include <iostream>

using namespace std;


void CapturedBlockTiming::init()
{
    block_gap = -1;
    prelude_lead_tone_cycles = -1;
    lead_tone_cycles = -1;
    micro_tone_cycles = -1;
    trailer_tone_cycles = -1;
    start = -1;
    end = -1;
    phase_shift = -1;
}

void CapturedBlockTiming::log()
{
    cout << "Block gap: " << dec << block_gap << "s\n";
    cout << "Prelude tone cycles: " << dec << prelude_lead_tone_cycles << "\n";
    cout << "Lead tone cycles: " << dec << lead_tone_cycles << "\n";
    cout << "Micro tone cycles: " << dec << block_gap << "\n";
    cout << "Trailer tone cycles : " << dec << trailer_tone_cycles << "\n";
    cout << "Start of block: " << dec << start << "s\n";
    cout << "End of block: " << dec << end << "s\n";
    cout << "Phaseshift: " << dec << phase_shift << "s\n";
}

void TapeProperties::log()
{
    cout << "Base frequency: " << dec << baseFreq << " Hz\n";
    cout << "Baudrate: " << dec << baudRate << " Hz\n";
    cout << "Phaseshift: " << dec << phaseShift << " degrees\n";
    cout << "Preserve timing: " << dec << preserve << "\n";
    cout << "\n";
    minBlockTiming.log("Min ");
    nomBlockTiming.log("Nominal ");
    cout << "\n";
   
}

void BlockTiming::log()
{
    log("");
}

void BlockTiming::log(string prefix)
{
    cout << prefix << "Prelude lead tone cycles: " << dec << firstBlockPreludeLeadToneCycles << "\n";
    cout << prefix << "First block lead tone duration: " << dec << firstBlockLeadToneDuration << "s\n";
    cout << prefix << "Other block lead tone duration: " << dec << otherBlockLeadToneDuration << "s\n";
    cout << prefix << "Micro tone duration: " << dec << microLeadToneDuration << "s\n";
    cout << prefix << "Trailer tone duration: " << dec << trailerToneDuration << "s\n";
    cout << prefix << "First block gap: " << dec << firstBlockGap << "s\n";
    cout << prefix << "Block gap: " << dec << blockGap << "s\n";
    cout << prefix << "Last block gap: " << dec << lastBlockGap << "s\n";
}