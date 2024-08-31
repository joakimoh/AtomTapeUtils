#include "BlockDecoder.h"
#include "../shared/WaveSampleTypes.h"
#include <iostream>
#include "../shared/Debug.h"
#include "../shared/Utility.h"


double BlockDecoder::getTime()
{
	return mCycleDecoder.getTime();
}

// Save the current file position
bool BlockDecoder::checkpoint()
{
	if (!mCycleDecoder.checkpoint()) {
		return false;
	}

	return true;
}

// Roll back to a previously saved file position
bool BlockDecoder::rollback()
{
	if (!mCycleDecoder.rollback()) {
		return false;
	}
	return true;
}

BlockDecoder::BlockDecoder(
	CycleDecoder& cycleDecoder, ArgParser& argParser, bool verbose, TargetMachine targetMachine) :
	mCycleDecoder(cycleDecoder), mArgParser(argParser), mVerbose(verbose), mTargetMachine(targetMachine)
{

	mTracing = argParser.tracing;

	if (!Utility::setBitTiming(mArgParser.tapeTiming.baudRate, argParser.targetMachine, mStartBitCycles,
		mLowDataBitCycles, mHighDataBitCycles, mStopBitCycles)
	) {

		throw invalid_argument("Unsupported baud rate " + mArgParser.tapeTiming.baudRate);
	}
	mDataBitSamples = round(mCycleDecoder.getF2Duration() * mHighDataBitCycles); // No of samples for a data bit

	// Threshold between no of 1/2 cycles for a '0' and '1' databit
	mDataBitHalfCycleBitThreshold = mLowDataBitCycles + mHighDataBitCycles;

	if (mVerbose) {
		cout << "#samples per data bit: " << mDataBitSamples << "\n";
		cout << "1/2 cycle threshold for data bit: " << mDataBitHalfCycleBitThreshold << "\n";
	}

	if (argParser.mErrorCorrection)
		mErrorCorrection = true;
}



bool BlockDecoder::checkByte(Byte refVal, Byte& readVal) {
	if (!getByte(&readVal) || readVal != refVal) {
		return false;
	}
	return true;
}



bool BlockDecoder::checkBytes(Byte refVal, int n) {
	bool failed = false;
	int start_cycle = 0;
	for (int i = start_cycle; i < n && !failed; i++) {
		Byte val = 0;
		if (!checkByte(refVal, val)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Byte #%d (%.2x) out of %d bytes differs from reference value 0x%.2x or couldn't be read!\n", i, val, n, refVal);
			failed = true;
		}
	}
	return !failed;
}


bool BlockDecoder::getBytes(Bytes& bytes, int n, Word &CRC) {
	bool failed = false;
	for (int i = 0; i < n && !failed; i++) {
		Byte byte;
		if (!getByte(&byte)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to read byte #%d out of %d bytes\n", i, n);
			failed = true;
		}
		Utility::updateCRC(mTargetMachine, CRC,byte);
		bytes.push_back(byte);
	}
	return !failed;
}


//
// Detect a start bit by looking for exactly mStartBitCycles low tone cycles
//
bool BlockDecoder::getStartBit()
{

	// Wait for mStartBitCycles * 2 "1/2 cycles" of low frequency F1 <=> start bit
	double waiting_time = 0;
	double t_start = getTime();
	int n_remaining_start_bit_half_cycles = mStartBitCycles * 2;
	bool one_cycle_found = false;

	// Detection of the last data bit of the previous byte might have run slightly into the start bit
	// In that case, the first F1 1/2 cycle of the start bit has already been recorded and it remains
	// only to detect mStartBitCycles * 2 - 1 1/2 cycles.
	if (mLastHalfCycleFrequency == Frequency::F1) {
		--n_remaining_start_bit_half_cycles;
		one_cycle_found = true;
	}
	DEBUG_PRINT(getTime(), DBG, "Last 1/2 Cycle was of type %s\n", _FREQUENCY(mLastHalfCycleFrequency));

	int  start_bit_detected = false;
	// Check for mStartBitCycles * 2 continuous F1 cycles (including the potentially already recorded one)
	while (!start_bit_detected) {
		double wt;
		if (!mCycleDecoder.stopOnHalfCycles(Frequency::F1, n_remaining_start_bit_half_cycles, wt, mLastHalfCycleFrequency)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to detect start bit\n", "");
			return false;
		}
		waiting_time += wt;
		if (wt > 0 && one_cycle_found) { // waiting => not continuous with already recorded F1 1/2 cycle => restart detection
			one_cycle_found = true;
			n_remaining_start_bit_half_cycles = mStartBitCycles * 2 - n_remaining_start_bit_half_cycles;
			DEBUG_PRINT(getTime(), ERR, "Waited %fs when a first F1 1/2 cycle had already been recorded => restart detection\n", wt);
		}
		else { // mStartBitCycles * 2 continuous 1/2 cycles detected
			start_bit_detected = true;
		}
	}
	
	DEBUG_PRINT(
		getTime(), DBG, 
		"start bit detected after waiting %d 1/2 F2 cycles before the first F1 cycle\n", round(waiting_time / (F2_FREQ * 2))
	);

	return true;
}


bool BlockDecoder::getDataBit(Bit& bit)
{
	int n_half_cycles;

	// Advance time corresponding to one bit and count the no of transitions (1/2 cycles)
	if (!mCycleDecoder.countHalfCycles(mDataBitSamples, n_half_cycles, mLastHalfCycleFrequency)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Unexpected end of samples when reading data bit\n");
		return false; // unexpected end of samples
	}

	// Decide whether the databit was a '0' or a '1' value based on the no of detected 1/2 cycles
	if (n_half_cycles < mDataBitHalfCycleBitThreshold)
		bit = LowBit;
	else
		bit = HighBit;

	DEBUG_PRINT(getTime(), DBG, "%d 1/2 cycles detected for data bit and therefore classified as a '%d'\n", n_half_cycles, bit);

	return true;

}

bool BlockDecoder::getWord(Word* word)
{
	Bytes bytes;
	Word dummy_CRC;
	if (!getBytes(bytes, 2, dummy_CRC)) {
		return false;
	}
	*word = bytes[0] * 256 + bytes[1];
	return true;
}


/*
 * Read a byte with bits in little endian order
*/
bool BlockDecoder::getByte(Byte* byte, int& nCollectedCycles)
{
	// Detect start bit
	if (!getStartBit()) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to read start bit%s\n", "");
		return false;
	}
	DEBUG_PRINT(getTime(), DBG, "Got start bit\n", "");

	// Get data bits
	*byte = 0;
	uint16_t f = 1;
	for (int i = 0; i < 8; i++) {
		Bit bit;
		if (!getDataBit(bit)) {
			if (mTracing)
				DEBUG_PRINT(getTime(), ERR, "Failed to read data bit b%d\n", i);
			return false;
		}
		*byte = *byte + bit * f;
		f = f * 2;
	}

	// No need to detect stop bit as the search for a start bit in the next
	// byte will continue past the stop bit.
	//getStopBit();

	nReadBytes++;

	DEBUG_PRINT(getTime(), DBG, "Got byte %.2x\n", *byte);

	return true;
}

bool BlockDecoder::getStopBit()
{
	// Get one cycle of high tone to make sure we're into stop bit before
	// searching for a start bit for the next byte

	double waiting_time;
	if (!mCycleDecoder.stopOnHalfCycles(Frequency::F2, 2, waiting_time, mLastHalfCycleFrequency)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to detect stop bit\n", "");
		return false;
	}

	DEBUG_PRINT(getTime(), DBG, "Stop bit consumed with last 1/2 cycle being %s\n", _FREQUENCY(mLastHalfCycleFrequency));

	return true;
}

bool BlockDecoder::getByte(Byte* byte)
{
	int collected_cycles;

	return getByte(byte, collected_cycles);

}
