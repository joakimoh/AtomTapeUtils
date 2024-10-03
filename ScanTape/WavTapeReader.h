#ifndef WAV_TAPE_READER_H
#define WAV_TAPE_READER_H

#include "TapeReader.h"
#include "../shared/WaveSampleTypes.h"
#include "CycleDecoder.h"
#include "../shared/FileBlock.h"
#include "../shared/BitTiming.h"


class WavTapeReader: public TapeReader {

private:

	double mDataSamples = 0.0;
	int mBitNo = 0;

	ArgParser mArgParser;

	CycleDecoder& mCycleDecoder;

	BitTiming mBitTiming;



	// Detect a start bit by looking for exactly mStartBitCycles low tone (F1) cycles
	bool getStartBit();
	bool getStartBit(bool restartAllowed);

	// Get a data bit
	bool getDataBit(Bit& bit);

	// Get a stop bit
	bool getStopBit();

	// Read a byte if possible
	bool readByte(Byte& byte, bool restartAllowed);

public:

	WavTapeReader(CycleDecoder& cycleDecoder, double baseFreq, ArgParser argParser);


	//
	// Virtual methods inherited from TapeReader parent class
	// 

	
	// Read a byte if possible
	bool readByte(Byte& byte);

	// Wait for at least minCycles of carrier
	bool waitForCarrier(int minCycles, double& waitingTime, int& cycles, AfterCarrierType afterCarrierType);

	bool waitForCarrierWithDummyByte(
		int minCycles, double& waitingTime, int& preludeCycles, int& postludecycles, Byte& foundDummyByte,
		AfterCarrierType afterCarrierType, bool detectDummyByte = true
	);

	// Consume a carrier of a min duration
	bool consumeCarrier(double minDuration, int& detectedCycles);

	// Get tape time
	double getTime();

	// Save the current file position
	bool checkpoint();

	// Roll back to a previously saved file position
	bool rollback();

	// Remove checkpoint (without rolling back)
	bool regretCheckpoint();

	// Get phase shift
	int getPhaseShift();

	// Return carrier frequency [Hz]
	double carrierFreq();
};

#endif