
#include "WavTapeReader.h"
#include "../shared/Debug.h"
#include "../shared/Utility.h"
#include <cmath>
#include <cstdint>

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
	return readByte(byte, true);
}

/*
 * Read a byte with bits in little endian order
*/
bool WavTapeReader::readByte(Byte& byte, bool restartAllowed)
{
	// Detect start bit
	if (!getStartBit(restartAllowed)) {
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

// Wait for at least minCycles of carrier
bool WavTapeReader::waitForCarrier(
	int minCycles, double& waitingTime, int& cycles, AfterCarrierType afterCarrierType
)
{
	int dummy_prelude_cycles = -1;
	Byte dummy_byte;
	// Call waitForCarrierPossiblyWithDummyByte with dummy byte detection disabled
	return waitForCarrierWithDummyByte(minCycles, waitingTime, dummy_prelude_cycles, cycles, dummy_byte, afterCarrierType, false);
}



bool WavTapeReader::waitForCarrierWithDummyByte(
	int minCycles, double& waitingTime, int& preludeCycles, int& postludecycles, Byte& foundDummyByte, 
	AfterCarrierType afterCarrierType, bool detectDummyByte
)
{
	int carrier_half_cycle_count = 0; // filtered number of detected carrier cycles
	int encountered_carrier_half_cycles = 0; // true number of detected carrier cycles
	double t_start = -1;
	int min_half_carrier_cycles = minCycles  * 2;
	double t_dummy_byte_start = -1;
	double t_dummy_byte = -1;
	preludeCycles = -1;
	double half_cycle_duration_acc = 0;
	double t_wait_start = getTime();
	string s;

	if (mVerbose) {
		s = (detectDummyByte ? " with dummy byte" : "");
		s += (afterCarrierType == STARTBIT_FOLLOWS ? " followed by STARTBIT" : " followed by GAP");
		cout << "Wait for carrier" << s << " of length " << dec << minCycles << " cycles(" <<
			(double)minCycles / carrierFreq() << "s) at " << Utility::encodeTime(getTime()) << "\n";
	}
	

	// Detect the min duration of the carrier
	while (carrier_half_cycle_count < min_half_carrier_cycles) {

		// Record time when carrier cycles starts to come
		if (t_start < 0 && carrier_half_cycle_count > 0) {
			t_start = getTime();
			waitingTime = getTime() - t_wait_start;
		} 
		else if (carrier_half_cycle_count == 0) {
			t_start = -1;
			t_dummy_byte_start = -1;
			t_dummy_byte = -1;
			preludeCycles = -1;
		}

		// Get next 1/2 cycle
		double t12_start = getTime();
		if (!mCycleDecoder.nextHalfCycle(mLastHalfCycleFrequency))
			return false; // End of tape reached if not 1/2 cycle could be detected => STOP

		double half_cycle_duration = getTime() - t12_start;

		// Increase carrier count for F2 cycle and decrease it for F1/undefined type of 1/2 cycles
		if (mLastHalfCycleFrequency == Frequency::F2) {
			carrier_half_cycle_count++;
			half_cycle_duration_acc += half_cycle_duration;
			encountered_carrier_half_cycles++;
		} 
		else
			carrier_half_cycle_count -= (int)round(half_cycle_duration * carrierFreq() * 2); // decrease = time passed in F2 units


		// Check for a dummy byte if at least two consecutive carrier 1/2 cycles have been detected before
		// the F1 1/2 cycle was detected. An F1 1/2 cycle means it could be the start bit of a dummy byte.
		if (detectDummyByte && mLastHalfCycleFrequency == Frequency::F1 && carrier_half_cycle_count > 2) {

			// Check for a dummy byte (0xaa)
			t_dummy_byte_start = getTime();
			double max_byte_duration = 1.5 * mBitTiming.F2CyclesPerByte / carrierFreq();
			if (readByte(foundDummyByte, false) && getTime() - t_dummy_byte_start < max_byte_duration) {
				// Dummy byte is normally 2/3 carrier frequency => leave carrier_half_cycle_count unchanged
				preludeCycles = (int)round((t_dummy_byte_start - t_start) * mCycleDecoder.carrierFreq());
				t_dummy_byte = getTime() - t_dummy_byte_start; // stop bits not included as not read for a byte
				int dummy_byte_half_cycles = (int)round(t_dummy_byte * carrierFreq() * 2);
			}
			else {
				double t_elapsed = getTime() - t_dummy_byte_start; // stop bits not included as not read for a byte
				int elapsed_half_cycles = (int)round(t_elapsed * carrierFreq() * 2);
				carrier_half_cycle_count -= elapsed_half_cycles;
			}

		}

		// If a gap (or an extremely long 1/2 cycle) was detected, the carrier 1/2 count might have to be reset
		if (carrier_half_cycle_count < 0)
			carrier_half_cycle_count = 0;
	}

	if (mVerbose) {
		if (t_dummy_byte_start != -1)
			cout << "Found dummy byte 0x" << hex << (int)foundDummyByte << "\n";

		cout << "Min carrier" << s << " of length " << (double)carrier_half_cycle_count / 2 << " cycles (" <<
			(double)carrier_half_cycle_count / 2 / carrierFreq() << "s) detected at " << Utility::encodeTime(getTime()) << "\n";
	}

	// Now advance until a start bit (one F1 1/2 cycle) is encountered (afterCarrierType = STARTBIT_FOLLOWS) or
	// a gap (afterCarrierType = GAP_FOLLOWS)
	Frequency stopType = Frequency::UndefinedFrequency;
	if (afterCarrierType == STARTBIT_FOLLOWS)
		stopType = Frequency::F1;
	double t12_start = getTime();
	checkpoint();
	while (mCycleDecoder.nextHalfCycle(mLastHalfCycleFrequency) && mLastHalfCycleFrequency != stopType) {

		// Create a checkpoint prior to the nexy nextHalfCycle() call to be able to remove the non-F2 cycle (i.e., an F1 cycle or a gap)
		// from the read tape stream before returning.
		regretCheckpoint();
		checkpoint();
		double half_cycle_duration = getTime() - t12_start;

		// Increase carrier count for F2 cycle
		if (mLastHalfCycleFrequency == Frequency::F2) {
			carrier_half_cycle_count++;
			half_cycle_duration_acc += half_cycle_duration;
			encountered_carrier_half_cycles++;
		}
		else
			// Decrease carrier for non-F2 cycle
			// Either Frequency::UndefinedFrequency <> Gap or very long 1/2 cycle (afterCarrierType == STARTBIT_FOLLOWS) or
			// Frequency::F1 (afterCarrierType == GAP_FOLLOWS)
			carrier_half_cycle_count -= (int)round(half_cycle_duration * carrierFreq()); // decrease = time passed in F2 units

		// If the carrier 1/2 cycle count goes below the min duration, then we've lost the carrier => ERROR
		if (carrier_half_cycle_count < min_half_carrier_cycles) {
			regretCheckpoint();
			return false;
		}

		// Remember time prior to the detection of the next 1/2 cycle
		t12_start = getTime();
	}
	Frequency stop_type = mLastHalfCycleFrequency;

	// Remove the last read start bit F2 cycle / gap
	rollback();
	if (mVerbose) {
		cout << "Detected carrier" << s << " of length " << (double)carrier_half_cycle_count / 2 << " cycles (" <<
			(double)carrier_half_cycle_count / 2 / carrierFreq() << "s) at " << Utility::encodeTime(getTime()) << "\n";

		cout << "Next 1/2 cycle is " << _FREQUENCY(mLastHalfCycleFrequency) << "\n";
	}

	// If no stopType frequency was detected, then the carrier was not followed by a stopType => ERROR
	if (stop_type != stopType) {
		return false;
	}

	double duration = getTime() - t_start;
	double prelude_duration = (double) preludeCycles / carrierFreq();
	if (preludeCycles > 0)
		postludecycles = (int) round((duration - prelude_duration - t_dummy_byte) * carrierFreq());
	else
		postludecycles = (int) round(duration * carrierFreq());

	double carrier_cycle_freq_av = 1 / (2 * half_cycle_duration_acc / encountered_carrier_half_cycles);
	double base_freq_av = carrier_cycle_freq_av / 2;

	// Update bit timing based on new measured carrier frequency
	if (mVerbose)
		cout << "Detected  a carrier frequency of " << carrier_cycle_freq_av << " - updating bit timing and cycle decoder with this!\n";
	mCycleDecoder.setCarrierFreq(carrier_cycle_freq_av);
	BitTiming updated_bit_timing(mCycleDecoder.getSampleFreq(), base_freq_av, mArgParser.tapeTiming.baudRate, mArgParser.targetMachine);
	mBitTiming = updated_bit_timing;

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

	// Add element to checkpoints
	mCheckpoints.push_back(mLastHalfCycleFrequency);
	return mCycleDecoder.checkpoint();
}

// Roll back to a previously saved file position
bool WavTapeReader::rollback()
{
	
	// Copy last element
	mLastHalfCycleFrequency = mCheckpoints.back();

	// Remove last element
	mCheckpoints.pop_back();
	return mCycleDecoder.rollback();
}

// Remove checkpoint (without rolling back)
bool WavTapeReader::regretCheckpoint()
{

	// Remove last element
	mCheckpoints.pop_back();
	return mCycleDecoder.regretCheckpoint();

}

//
// Detect a start bit by looking for exactly mStartBitCycles low tone cycles
//
bool WavTapeReader::getStartBit()
{
	return getStartBit(true);
}
//
// Detect a start bit by looking for exactly mStartBitCycles low tone cycles
//
bool WavTapeReader::getStartBit(bool restartAllowed)
{

	mDataSamples = 0.0;
	mBitNo = 0;

	// Wait for mStartBitCycles * 2 "1/2 cycles" of low frequency F1 <=> start bit
	double waiting_time = 0;
	double t_start = getTime();
	int n_remaining_start_bit_half_cycles = mBitTiming.startBitCycles * 2;
	bool one_cycle_found = false;

	// Detection of the last data bit of the previous byte might have run slightly into the start bit
	// In that case, the first F1 1/2 cycle of the start bit has already been recorded and it remains
	// only to detect mStartBitCycles * 2 - 1 1/2 cycles.
	int n = n_remaining_start_bit_half_cycles;
	if (mLastHalfCycleFrequency == Frequency::F1) {
		--n_remaining_start_bit_half_cycles;
		one_cycle_found = true;
	}
	DEBUG_PRINT(getTime(), DBG, "Last 1/2 Cycle was of type %s, waiting for %d 1/2 cycles of F1\n", _FREQUENCY(mLastHalfCycleFrequency),n_remaining_start_bit_half_cycles);

	// If the start bit needs to be continuous with already recorded F1 1/2 cycle, then just read F1 cycles

	if (!restartAllowed) {
		DEBUG_PRINT(getTime(), DBG, "Wait for a continuous sequence of %d F1 1/2 cycles\n", n_remaining_start_bit_half_cycles);
		while (n_remaining_start_bit_half_cycles > 0 && mLastHalfCycleFrequency == Frequency::F1 &&
			mCycleDecoder.nextHalfCycle(mLastHalfCycleFrequency)) {
			--n_remaining_start_bit_half_cycles;
		}
		if (n_remaining_start_bit_half_cycles != 0)
			return false;
		return true;
	}

	// If the start bit doesn't have to be continuous with already recorded F1 1/2 cycle, then allow restarts of detection until
	// a valid start bit is detected.
	
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
		"start bit detected after waiting %d 1/2 F2 cycles before the first F1 cycle\n",
		(int)round(waiting_time / (mCycleDecoder.carrierFreq() * 2))
	);

	return true;
}

bool WavTapeReader::getDataBit(Bit& bit)
{
	int n_half_cycles;
	int n_bit_samples = (int) (round(mDataSamples+ mBitTiming.dataBitSamples) - round(mDataSamples));
	mDataSamples += mBitTiming.dataBitSamples;
	mBitNo++;
	double t_start = getTime();

	// Advance time corresponding to one bit and count the no of transitions (1/2 cycles)
	int max_half_cycle_duration;
	if (!mCycleDecoder.countHalfCycles(n_bit_samples, n_half_cycles, max_half_cycle_duration, mLastHalfCycleFrequency)) {
		if (mTracing)
			DEBUG_PRINT(getTime(), ERR, "Unexpected end of samples when reading data bit%s\n", "");
		return false; // unexpected end of samples
	}

	// Decide whether the databit was a '0' or a '1' value based on the no of detected 1/2 cycles and the max 1/2 cycle duration
	if (n_half_cycles <= mBitTiming.dataBitHalfCycleBitThreshold && mCycleDecoder.strictValidHalfCycleRange(F1, max_half_cycle_duration))
		bit = LowBit;
	else {
		bit = HighBit;
		double t_end = getTime();
		if (mVerbose && !(n_half_cycles >= mBitTiming.dataBitHalfCycleBitThreshold && mCycleDecoder.strictValidHalfCycleRange(F2, max_half_cycle_duration))
			&& t_end > 10) {
			cout << "*** WARNING *** Invalid 1/2 cycles: " << n_half_cycles <<
				" detected with max duration " << max_half_cycle_duration << " for " << n_bit_samples << " samples and at "
				<< Utility::encodeTime(t_start) << " => " << Utility::encodeTime(t_end) << "\n";
				mCycleDecoder.mCT.log();
				mBitTiming.log();
		}
	}

	DEBUG_PRINT(getTime(), DBG, "%d 1/2 cycles (of max duration %d) detected for data bit and therefore classified as a '%d'\n", n_half_cycles, max_half_cycle_duration, bit);

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

// Return carrier frequency [Hz]
double WavTapeReader::carrierFreq()
{
	return mCycleDecoder.carrierFreq();
}