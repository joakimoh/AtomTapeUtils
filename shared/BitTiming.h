#ifndef BIT_TIMING_H
#define BIT_TIMING_H

#include "WaveSampleTypes.h"
#include "FileBlock.h"

class BitTiming {

private:

    bool setBitTiming(int sampleFreq, double baseFreq, int baudRate, TargetMachine targetMachine);

public:

    BitTiming(int sampleFreq, double baseFreq, int baudRate, TargetMachine targetMachine);
    BitTiming() {}
    
    void log();
    
    // Default values below is for Acorn Atom, 44.1 kHz sample rate and 300 Baud
    
    int startBitCycles = 4; // Start bit length in cycles of F1 frequency carrier
    int lowDataBitCycles = 4; // Data bit length in cycles of F1 frequency carrier
    int highDataBitCycles = 8; // Data bit length in cycles of F2 frequency carrier
    int stopBitCycles = 9; // Stop bit length in cycles of F2 frequency carrier

    double dataBitSamples = 44100 / 2400 * 8; // duration (in samples) of a data bit
    int dataBitHalfCycleBitThreshold = 12; // threshold (in 1/2 cycles) between a '0' and a '1'

    double F2Samples = 44100 / 2400; // No of samples for a complete F2 cycle
    double F1Samples = 44100 / 1200; // No of samples for a complete F1 cycle

    int fS = 44100; // sample frequency

    int F2CyclesPerByte = 8 + 8 * 8 + 9; // start bit + 8 data bits + 1 stop bit (with extra wave)
  
};

#endif