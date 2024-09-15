#include "BitTiming.h"
#include <iostream>

BitTiming::BitTiming(int sampleFreq, double baseFreq, int baudRate, TargetMachine targetMachine)
{
    setBitTiming(sampleFreq, baseFreq, baudRate, targetMachine);
}


bool BitTiming::setBitTiming(int sampleFreq, double baseFreq, int baudRate, TargetMachine targetMachine)
{

    fS = sampleFreq;

    if (baudRate == 300) {
        startBitCycles = 4; // Start bit length in cycles of F1 frequency carrier
        lowDataBitCycles = 4; // Data bit length in cycles of F1 frequency carrier
        highDataBitCycles = 8; // Data bit length in cycles of F2 frequency carrier
        if (targetMachine <= BBC_MASTER || targetMachine == UNKNOWN_TARGET)
            stopBitCycles = 8; // Stop bit length in cycles of F2 frequency carrier
        else // ACORN_ATOM
            stopBitCycles = 9;      
    }
    else if (baudRate == 1200) {
        startBitCycles = 1; // Start bit length in cycles of F1 frequency carrier
        lowDataBitCycles = 1; // Data bit length in cycles of F1 frequency carrier
        highDataBitCycles = 2; // Data bit length in cycles of F2 frequency carrier
        if (targetMachine <= BBC_MASTER || targetMachine == UNKNOWN_TARGET)
            stopBitCycles = 2; // Stop bit length in cycles of F2 frequency carrier
        else // ACORN_ATOM
            stopBitCycles = 3;
    }
    else {
        cout << "Invalid baud rate " << baudRate << "!\n";
        return false;
    }

    dataBitSamples = (int) round( (double)sampleFreq / F2_FREQ * highDataBitCycles); // No of samples for a data bit

    // Threshold between no of 1/2 cycles for a '0' and '1' databit
    dataBitHalfCycleBitThreshold = lowDataBitCycles + highDataBitCycles;

    F2Samples = (double) fS / (2 * baseFreq);
    F1Samples = (double) fS / baseFreq;

    F2CyclesPerByte = startBitCycles * 2 + highDataBitCycles * 8 + stopBitCycles;

    return true;
}

void BitTiming::log()
{
    cout << "Sample frequecny: " << fS << " Hz\n";
    cout << "startBitCycles: " << startBitCycles << "\n";
    cout << "lowDataBitCycles: " << lowDataBitCycles << "\n";
    cout << "highDataBitCycles: " << highDataBitCycles << "\n";
    cout << "stopBitCycles: " << stopBitCycles << "\n";
    cout << "F2Samples: " << F2Samples << "\n";
    cout << "F1Samples: " << F1Samples << "\n";
    cout << "F2CyclesPerByte: " << F2CyclesPerByte << "\n";
    cout << "\n";
}