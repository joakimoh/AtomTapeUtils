#include "UEFTapeReader.h"
#include "../shared/Debug.h"
#include "../shared/Utility.h"


UEFTapeReader::UEFTapeReader(UEFCodec& uefCodec, ArgParser &argParser) :
	mArgParser(argParser), TapeReader(argParser.verbose, argParser.tracing), mUEFCodec(uefCodec),
	mTargetMachine(argParser.targetMachine)
{
	mUEFCodec.readUefFile(argParser.wavFile);
}


/*
 * Read a byte with bits in little endian order
*/
bool UEFTapeReader::readByte(Byte& byte)
{
	Bytes data;
	
	if (!mUEFCodec.readfromDataChunk(1, data) || data.size() == 0) {
		return false;
	}
	byte = data[0] ;

	DEBUG_PRINT(getTime(), DBG, "Got byte %.2x\n", byte);

	return true;
}


// Wait for at least minCycles of carrier
bool UEFTapeReader::waitForCarrier(int minCycles, double& waitingTime, int& cycles)
{
	// Wait for lead tone of a min duration but still 'consume' the complete tone (including dummy byte if applicable)
	double duration;

	double min_duration = (double) minCycles / carrierFreq();
	if (!mUEFCodec.detectCarrier(waitingTime, duration) || duration < min_duration) {
		return false;
	}

	if (mVerbose)
		cout << duration << "s lead tone detected at " << Utility::encodeTime(getTime()) << " after wating " << waitingTime << "s\n";

	cycles = (int) round(duration * carrierFreq());

	return true;
}

// Consume a carrier of min duration and record its duration (no waiting for carrier)
bool UEFTapeReader::consumeCarrier(double minDuration, int& detectedCycles)
{
	double duration;
	if (!mUEFCodec.detectCarrier(duration) || duration < minDuration) {
		return false;
	}

	detectedCycles = (int) round(duration * carrierFreq());

	return true;
}

// Wait for at least minPreludeCycles + minPostludeCycles cycles of carrier with dummy byte (0xaa)
bool UEFTapeReader::waitForCarrierWithDummyByte(
	int minPreludeCycles, int minPostludeCycles, Byte dummyByte, double& waitingTime, int& preludeCycles, int& postludecycles
) {
	double duration1, duration2;

	// Wait for lead tone of a min duration (with or without a dummy byte)
	double min_prelude_duration = (double)minPreludeCycles / carrierFreq();
	double min_postlude_duration = (double)postludecycles / carrierFreq();
	if (!mUEFCodec.detectCarrierWithDummyByte(waitingTime, duration1, duration2) || duration1 < min_prelude_duration) {
		cout << "Failed to detect a carrier (with or without a dummy byte) at " << Utility::encodeTime(getTime()) << "\n";
		return false;
	}

	if (mVerbose && duration2 == -1)
		cout << duration2 << "s prelude lead tone detected at " << Utility::encodeTime(getTime()) << "\n";
	else if (mVerbose) {
		cout << minPreludeCycles << " cycles prelude tone and dummy byte 0xaa followed by postlude tone of " << duration2 << "s\n";
	}

	// Continue if it was a carrier chunk without a dummy byte
	if (duration2 == -1) {
		Bytes dummy_byte_data;
		if (!mUEFCodec.readfromDataChunk(1, dummy_byte_data) || dummy_byte_data.size() != 1) {
			cout << "Failed to read dummy byte at " << Utility::encodeTime(getTime()) << "\n";
			return false;
		}

		if (mVerbose)
			cout << "dummy byte " << hex << (int) dummy_byte_data[0] << " detected at " << Utility::encodeTime(getTime()) << "\n";

		if (!mUEFCodec.detectCarrier(duration2) && duration2 < min_postlude_duration) {
			cout << "Failed to detect a carrier after the dummy byte at " << Utility::encodeTime(getTime()) << "\n";
			return false;
		}

		if (mVerbose)
			cout << duration2 << "s postlude lead tone detected at " << Utility::encodeTime(getTime()) << "\n";
	}

	preludeCycles = (int)round(duration1 * carrierFreq());
	postludecycles = (int)round(duration2 * carrierFreq());


	return true;
}

// Get tape time
double UEFTapeReader::getTime()
{
	return mUEFCodec.getTime();
}

// Save the current file position
bool UEFTapeReader::checkpoint()
{
	if (!mUEFCodec.checkpoint()) {
		return false;
	}

	return true;
}

// Roll back to a previously saved file position
bool UEFTapeReader::rollback()
{
	if (!mUEFCodec.rollback()) {
		return false;
	}
	return true;
}

// Get phase shift
int UEFTapeReader::getPhaseShift()
{
	return mUEFCodec.getPhaseShift();
}

// Get duration of one carrier cycle
double UEFTapeReader::carrierFreq()
{
	return mUEFCodec.getBaseFreq() * 2;
}