

#include "WavTapeReader.h"
#include "../shared/Debug.h"
#include "../shared/Utility.h"

WavTapeReader::WavTapeReader(CycleDecoder& cycleDecoder, double baseFreq, ArgParser argParser) :
	mArgParser(argParser), TapeReader(argParser.verbose, argParser.tracing), mCycleDecoder(cycleDecoder),
	mBitTiming(cycleDecoder.getSampleFreq(), baseFreq, argParser.tapeTiming.baudRate, argParser.targetMachine)
{

}


/*
 * Read a byte with bits in little endian order
*/
bool WavTapeReader::readByte(Byte& byte)
{
	// Detect start bit
	if (!getStartBit()) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read start bit%s\n", "");
		return false;
	}
	DEBUG_PRINT(getTime(), DBG, "Got start bit%s\n", "");

	// Get data bits
	byte = 0;
	uint16_t f = 1;
	for (int i = 0; i < 8; i++) {
		Bit bit;
		if (!getDataBit(bit)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to read data bit b%d\n", i);
			return false;
		}
		byte = byte + bit * f;
		f = f * 2;
	}

	// No need to detect stop bit as the search for a start bit in the next
	// byte will continue past the stop bit.
	//getStopBit();


	DEBUG_PRINT(getTime(), DBG, "Got byte %.2x\n", byte);

	return true;
}


// Wait for at least minCycles of carrier
bool WavTapeReader::waitForCarrier(int minCycles, double& waitingTime, int& cycles)
{
	// Wait for lead tone of a min duration but still 'consume' the complete tone (including dummy byte if applicable)
	double duration;

	double lead_tone_duration = (double) minCycles / F2_FREQ;
	if (!mCycleDecoder.waitForTone(lead_tone_duration, duration, waitingTime, cycles, mLastHalfCycleFrequency)) {
		// This is not necessarily an error - it could be because the end of the tape as been reached...
		return false;
	}
	if (mVerbose)
		cout << duration << "s lead tone detected at " << Utility::encodeTime(getTime()) << " after wating " << waitingTime << "s\n";

	return true;
}

// Consume a carrier of min duration and record its duration (no waiting for carrier)
bool WavTapeReader::consumeCarrier(double minDuration, int& detectedCycles)
{
	double waiting_time;
	int detected_half_cycles = 2;
	if (!mCycleDecoder.stopOnHalfCycles(Frequency::F2, detected_half_cycles, waiting_time, mLastHalfCycleFrequency)) { // Find the  first 2 1/2 cycles
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to detect a carrier of min duration %f s\n", minDuration);
		return false;
	}
	detectedCycles = 2; // The first found 1/2 cycles
	if (!mCycleDecoder.consumeHalfCycles(Frequency::F2, detected_half_cycles, mLastHalfCycleFrequency)) { // Consume the remaining 1/2 cycles
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to detect a carrier of min duration %f s\n", minDuration);
		return false;
	}

	detectedCycles = detected_half_cycles / 2;

	return true;
}

// Wait for at least minPreludeCycles + minPostludeCycles cycles of carrier with dummy byte (0xaa)
bool WavTapeReader::waitForCarrierWithDummyByte(
	int minPreludeCycles, int minPostludeCycles, Byte dummyByte, double& waitingTime, int& preludeCycles, int& postludecycles
) {

	// Wait for lead tone of a min duration but still 'consume' the complete tone (including dummy byte if applicable)
	double duration;

	double prelude_tone_duration = (double) minPreludeCycles / F2_FREQ;
	if (!mCycleDecoder.waitForTone(prelude_tone_duration, duration, waitingTime, preludeCycles, mLastHalfCycleFrequency)) {
		// This is not necessarily an error - it could be because the end of the tape as been reached...
		return false;
	}
	if (mVerbose)
		cout << duration << "s prelude lead tone detected at " << Utility::encodeTime(getTime()) << "\n";

	// Skip dummy byte
	Byte dummy_byte = 0x00;
	if (!readByte(dummy_byte) || dummy_byte != 0xaa) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read dummy byte (0xaa byte)%s\n", "");
		//return false;
	}
	if (mVerbose)
		cout << "dummy byte 0x" << hex << (int) dummy_byte << " (as part of lead carrier) detected at " << Utility::encodeTime(getTime()) << "\n";

	// Get postlude part of lead tone
	double postlude_tone_duration = (double)minPostludeCycles / F2_FREQ;
	double waiting_time;
	if (!mCycleDecoder.waitForTone(postlude_tone_duration, duration, waiting_time, postludecycles, mLastHalfCycleFrequency)) {
		// This is not necessarily an error - it could be because the end of the tape as been reached...
		return false;
	}
	
	duration = (double) postludecycles / F2_FREQ;
	if (mVerbose)
		cout << duration << "s postlude lead tone detected at " << Utility::encodeTime(getTime()) << "\n";

	return true;
}

// Get tape time
double WavTapeReader::getTime()
{
	return mCycleDecoder.getTime();
}

// Save the current file position
bool WavTapeReader::checkpoint()
{
	if (!mCycleDecoder.checkpoint()) {
		return false;
	}

	return true;
}

// Roll back to a previously saved file position
bool WavTapeReader::rollback()
{
	if (!mCycleDecoder.rollback()) {
		return false;
	}
	return true;
}

//
// Detect a start bit by looking for exactly mStartBitCycles low tone cycles
//
bool WavTapeReader::getStartBit()
{

	// Wait for mStartBitCycles * 2 "1/2 cycles" of low frequency F1 <=> start bit
	double waiting_time = 0;
	double t_start = getTime();
	int n_remaining_start_bit_half_cycles = mBitTiming.startBitCycles * 2;
	bool one_cycle_found = false;

	// Detection of the last data bit of the previous byte might have run slightly into the start bit
	// In that case, the first F1 1/2 cycle of the start bit has already been recorded and it remains
	// only to detect mStartBitCycles * 2 - 1 1/2 cycles.
	if (mLastHalfCycleFrequency == Frequency::F1) {
		--n_remaining_start_bit_half_cycles;
		one_cycle_found = true;
	}
	DEBUG_PRINT(getTime(), DBG, "Last 1/2 Cycle was of type %s, waiting for %d 1/2 cycles of F1\n", _FREQUENCY(mLastHalfCycleFrequency),n_remaining_start_bit_half_cycles);

	int  start_bit_detected = false;
	// Check for mStartBitCycles * 2 continuous F1 cycles (including the potentially already recorded one)
	while (!start_bit_detected) {
		double wt;
		if (!mCycleDecoder.stopOnHalfCycles(Frequency::F1, n_remaining_start_bit_half_cycles, wt, mLastHalfCycleFrequency)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to detect start bit%s\n", "");
			return false;
		}
		waiting_time += wt;
		if (wt > 0 && one_cycle_found) { // waiting => not continuous with already recorded F1 1/2 cycle => restart detection
			one_cycle_found = true;
			n_remaining_start_bit_half_cycles = mBitTiming.startBitCycles * 2 - n_remaining_start_bit_half_cycles;
			DEBUG_PRINT(getTime(), ERR, "Waited %fs when a first F1 1/2 cycle had already been recorded => restart detection\n", wt);
		}
		else { // mStartBitCycles * 2 continuous 1/2 cycles detected
			start_bit_detected = true;
		}
	}

	DEBUG_PRINT(
		getTime(), DBG,
		"start bit detected after waiting %d 1/2 F2 cycles before the first F1 cycle\n", (int)round(waiting_time / (F2_FREQ * 2))
	);

	return true;
}

bool WavTapeReader::getDataBit(Bit& bit)
{
	int n_half_cycles;

	// Advance time corresponding to one bit and count the no of transitions (1/2 cycles)
	if (!mCycleDecoder.countHalfCycles(mBitTiming.dataBitSamples, n_half_cycles, mLastHalfCycleFrequency)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Unexpected end of samples when reading data bit%s\n", "");
		return false; // unexpected end of samples
	}

	// Decide whether the databit was a '0' or a '1' value based on the no of detected 1/2 cycles
	if (n_half_cycles < mBitTiming.dataBitHalfCycleBitThreshold)
		bit = LowBit;
	else
		bit = HighBit;

	DEBUG_PRINT(getTime(), DBG, "%d 1/2 cycles detected for data bit and therefore classified as a '%d'\n", n_half_cycles, bit);

	return true;

}

bool WavTapeReader::getStopBit()
{
	// Get one cycle of high tone to make sure we're into stop bit before
	// searching for a start bit for the next byte

	double waiting_time;
	if (!mCycleDecoder.stopOnHalfCycles(Frequency::F2, 2, waiting_time, mLastHalfCycleFrequency)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to detect stop bit%s\n", "");
		return false;
	}

	DEBUG_PRINT(getTime(), DBG, "Stop bit consumed with last 1/2 cycle being %s\n", _FREQUENCY(mLastHalfCycleFrequency));

	return true;
}

// Get phase shift
int WavTapeReader::getPhaseShift()
{
	return mCycleDecoder.getPhaseShift();
}

// Get duration of one carrier cycle
double WavTapeReader::carrierFreq()
{
	return mCycleDecoder.getF2Duration();
}