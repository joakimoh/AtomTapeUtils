#ifndef UEF_TAPE_READER_H
#define UEF_TAPE_READER_H

#include "TapeReader.h"
#include "../shared/WaveSampleTypes.h"
#include "../shared/FileBlock.h"
#include "../shared/UEFCodec.h"
#include "ArgParser.h"

class UEFTapeReader : public TapeReader {

protected:

	ArgParser mArgParser;
	UEFCodec &mUEFCodec;
	TargetMachine mTargetMachine;


public:

	UEFTapeReader(UEFCodec& uefCodec, ArgParser &argParser);

	//
	// Virtual methods inherited from TapeReader parent class
	// 


	// Read a byte if possible
	bool readByte(Byte& byte);

	// Wait for at least minCycles of carrier
	bool waitForCarrier(int minCycles, double& waitingTime, int& cycles);

	// Wait for at least minPreludeCycles + minPostludeCycles cycles of carrier with dummy byte (0xaa)
	bool waitForCarrierWithDummyByte(
		int minPreludeCycles, int minPostludeCycles, Byte dummyByte, double& waitingTime, int& preludeCycles, int& postludecycles
	);

	// Consume a carrier of a min duration
	bool consumeCarrier(double minDuration, int& detectedCycles);

	// Get tape time
	double getTime();

	// Save the current file position
	bool checkpoint();

	// Roll back to a previously saved file position
	bool rollback();

	// Get phase shift
	int getPhaseShift();

	// Return carrier frequency [Hz]
	double carrierFreq();
};

#endif