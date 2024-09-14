#ifndef TAPE_READER_H
#define TAPE_READER_H

#include "CommonTypes.h"
#include "FileBlock.h"


class TapeReader {

protected:

	bool mVerbose = false;

	bool mTracing = false;

	Word mCRC = 0;

public:

	// Read a byte if possible
	virtual bool readByte(Byte &byte) = 0;

	// Read n bytes if possible
	bool readBytes(Bytes &data, int n);
	bool readBytes(Bytes& data, int n, int& read_bytes);

	// Read min nMin bytes, up to nMax, and until a terminator is encountered
	bool readString(string &bs, int nMin, int nMax, Byte terminator, int &n);

	// Wait for at least minCycles of carrier
	virtual bool waitForCarrier(int minCycles, double& waitingTime, int& cycles) = 0;

	// Wait for at least minPreludeCycles + minPostludeCycles cycles of carrier with dummy byte (0xaa)
	virtual bool waitForCarrierWithDummyByte(
		int minPreludeCycles, int minPostludeCycles, Byte dummyByte, double& waitingTime, int& preludeCycles, int& postludecycles
	) = 0;

	// Consume a carrier of min duration and record its duration (no waiting for carrier)
	virtual bool consumeCarrier(double minDuration, int& detectedCycles) = 0;

	// Get phase shift
	virtual int getPhaseShift() = 0;

	// Constructor
	TapeReader(bool verbose, bool tracing): mVerbose(verbose), mTracing(tracing) {}

	// Get tape time
	virtual double getTime() = 0;

	// Save the current file position
	virtual bool checkpoint() = 0;

	// Roll back to a previously saved file position
	virtual bool rollback() = 0;

	// Get duration of one carrier cycle
	virtual double carrierFreq() = 0;

};

#endif