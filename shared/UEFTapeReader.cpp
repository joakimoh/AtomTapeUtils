#include "UEFTapeReader.h"
#include "Logging.h"
#include "Utility.h"
#include <cmath>


UEFTapeReader::UEFTapeReader(UEFCodec& uefCodec, string file, Logging logging, TargetMachine targetMachine
) :
	TapeReader(targetMachine, logging), mUEFCodec(uefCodec)
{
	mUEFCodec.readUefFile(file);
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
bool UEFTapeReader::waitForCarrier(int minCycles, double& waitingTime, int& cycles, AfterCarrierType afterCarrierType)
{
	// Wait for lead tone of a min duration but still 'consume' the complete tone (including dummy byte if applicable)
	double duration;

	double min_duration = (double) minCycles / carrierFreq();
	if (!mUEFCodec.detectCarrier(waitingTime, duration) || duration < min_duration) {
		return false;
	}

	if (mDebugInfo.verbose)
		cout << duration << "s lead tone detected at " << Utility::encodeTime(getTime()) << " after waiting " << waitingTime << "s\n";

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

// Wait for at least minCycles cycles of carrier, possibly with a dummy byte (0xaa)
bool UEFTapeReader::waitForCarrierWithDummyByte(
	int minCycles, double& waitingTime, int& preludeCycles, int& postludecycles, Byte& foundDummyByte,
	AfterCarrierType afterCarrierType, bool detectDummyByte
)
{
	double duration1, duration2;


	double min_duration = (double)minCycles / carrierFreq();

	// Wait for lead tone of a min duration (with or without a dummy byte)
	if (!mUEFCodec.detectCarrierWithDummyByte(waitingTime, duration1, duration2)) {
		// Could be and of tape and not an error
		// cout << "Failed to detect a carrier (with or without a dummy byte) at " << Utility::encodeTime(getTime()) << "\n";
		return false;
	}
	
	// If the carrier was without a dummy byte but exceeded the min duration, then completed (dummy byte is optional)
	if (duration2 == -1 && duration1 >= min_duration) {
		preludeCycles = -1;
		postludecycles = (int)round(duration1 * carrierFreq());
		if (mDebugInfo.verbose)
			cout << duration1 << "s carrier without dummy byte detected at " << Utility::encodeTime(getTime()) << "\n";
		return true;
	}

	// Check if the carrier included a dummy byte
	if (duration1 != -1 && duration2 != -1) {
		if (duration1 + duration2 > min_duration) {
			// The carrier included a dummy byte and exceeded the min duration => completed
			preludeCycles = (int)round(duration1 * carrierFreq());
			postludecycles = (int)round(duration2 * carrierFreq());
			if (mDebugInfo.verbose)
				cout << preludeCycles << " cycles prelude tone, an implicit dummy byte (0xaa) followed by a " << duration2 << "s postlude tone\n";
			return true;
		}
		else
			// Too short carrier with dummy byte => FAILED
			return false;
	}

	// The carrier didn't include a dummy byte and has a duration below the min duration => assume an explicit dummy byte will follow


	Bytes dummy_byte_data;
	if (!mUEFCodec.readfromDataChunk(1, dummy_byte_data) || dummy_byte_data.size() != 1) {
		// Could be and of tape and not necessary an error
		cout << "Failed to read dummy byte at " << Utility::encodeTime(getTime()) << "\n";
		return false;
	}

	if (mDebugInfo.verbose)
		cout << "dummy byte " << hex << (int) dummy_byte_data[0] << " detected at " << Utility::encodeTime(getTime()) << "\n";

	if (!mUEFCodec.detectCarrier(duration2) && duration1 + duration2 < min_duration) {
		cout << "Failed to detect a long enough carrier (" << duration1 << "+" << duration2 << "/" << min_duration << "s) after the dummy byte at " << Utility::encodeTime(getTime()) << "\n";
		return false;
	}

	preludeCycles = (int)round(duration1 * carrierFreq());
	postludecycles = (int)round(duration2 * carrierFreq());

	if (mDebugInfo.verbose) {
		cout << preludeCycles << " cycles prelude tone, a dummy byte 0x" << hex << (int)dummy_byte_data[0] <<
			dec << " followed by a " << duration2 << "s postlude tone\n";
	}
	

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
	return mUEFCodec.rollback();
}

// Remove checkpoint (without rolling back)
bool UEFTapeReader::regretCheckpoint()
{
	return mUEFCodec.regretCheckpoint();
}

// Get phase shift
int UEFTapeReader::getPhaseShift()
{
	return mUEFCodec.getPhaseShift();
}

// Return carrier frequency [Hz]
double UEFTapeReader::carrierFreq()
{
	return mUEFCodec.getBaseFreq() * 2;
}