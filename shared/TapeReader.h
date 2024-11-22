#ifndef TAPE_READER_H
#define TAPE_READER_H

#include "CommonTypes.h"
#include "FileBlock.h"
#include "Logging.h"


enum AfterCarrierType { GAP_FOLLOWS = 0x1, HEADER_FOLLOWS = 0x2, DATA_FOLLOWS = 0x4, START_BIT_FOLLOWS = 0x6 };
#define _CARRIER_TYPE(x) (\
	x==AfterCarrierType::GAP_FOLLOWS?"Gap":\
		(x==AfterCarrierType::HEADER_FOLLOWS?"Header":\
			(x==AfterCarrierType::DATA_FOLLOWS?"Data":"Header/Data")\
		)\
)

class TapeReader {

protected:

	Logging mDebugInfo;

	TargetMachine mTargetMachine;

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
	virtual bool waitForCarrier(int minCycles, double& waitingTime, int& cycles, AfterCarrierType afterCarrierType) = 0;

	virtual bool waitForCarrierWithDummyByte(
		int minCycles, double& waitingTime, int& preludeCycles, int& postludecycles, Byte& foundDummyByte, 
		AfterCarrierType afterCarrierType, bool detectDummyByte = true
		 
	) = 0;

	// Consume a carrier of min duration and record its duration (no waiting for carrier)
	virtual bool consumeCarrier(double minDuration, int& detectedCycles) = 0;

	// Get phase shift
	virtual int getPhaseShift() = 0;

	// Constructor
	TapeReader(TargetMachine targetMachine, Logging logging):
		mTargetMachine(targetMachine), mDebugInfo(logging) {}

	// Get tape time
	virtual double getTime() = 0;

	// Save the current file position
	virtual bool checkpoint() = 0;

	// Roll back to a previously saved file position
	virtual bool rollback() = 0;

	// Remove checkpoint (without rolling back)
	virtual bool regretCheckpoint() = 0;

	// Return carrier frequency [Hz]
	virtual double carrierFreq() = 0;

};

#endif