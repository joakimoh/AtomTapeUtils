
#include "WavTapeReader.h"
#include "Logging.h"
#include "Utility.h"
#include "TapeProperties.h"
#include <cmath>
#include <cstdint>

WavTapeReader::WavTapeReader(
	CycleDecoder& cycleDecoder, double baseFreq, TapeProperties tapeTiming, TargetMachine targetMachine, Logging logging
) : TapeReader(targetMachine, logging), mCycleDecoder(cycleDecoder),
	mBitTiming(cycleDecoder.getSampleFreq(), baseFreq, tapeTiming.baudRate, targetMachine),
	mTapeTiming(tapeTiming)
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
		if (mDebugInfo.tracing)
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
			if (mDebugInfo.tracing)
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
	//cout << "Got byte 0x" << hex << (int)byte << " at " << Utility::encodeTime(getTime()) << "\n";

	return true;
}

// Consume a carrier of min duration and record its duration (no waiting for carrier)
bool WavTapeReader::consumeCarrier(double minDuration, int& detectedCycles)
{
	double waiting_time;
	int detected_half_cycles = 2;
	if (!mCycleDecoder.stopOnHalfCycles(Frequency::F2, detected_half_cycles, waiting_time)) { // Find the  first 2 1/2 cycles
		if (mDebugInfo.tracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to detect a carrier of min duration %f s\n", minDuration);
		return false;
	}
	detectedCycles = 2; // The first found 1/2 cycles
	if (!mCycleDecoder.consumeHalfCycles(Frequency::F2, detected_half_cycles)) { // Consume the remaining 1/2 cycles
		if (mDebugInfo.tracing)
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
	// Set the acceptance of a premature gap/header/data to half that of the minCycle
	const int carrier_duration_threshold_half_cycles = minCycles / 4;

	int carrier_half_cycle_count = 0; // filtered number of detected carrier cycles
	int encountered_carrier_half_cycles = 0; // true number of detected carrier cycles
	int min_half_carrier_cycles = minCycles  * 2;
	double t_dummy_byte_start = -1;
	double t_dummy_byte = -1;
	preludeCycles = -1;
	double half_cycle_duration_acc = 0;
	double t_wait_start = getTime();
	string s;
	bool detected_dummy_byte = false;

	if (mDebugInfo.verbose) {
		s = (detectDummyByte ? " with dummy byte" : "");
		s = s + " followed by " + _CARRIER_TYPE(afterCarrierType);
		cout << "Wait for carrier" << s << " of length " << dec << minCycles << " cycles(" <<
			(double)minCycles / carrierFreq() << "s) at " << Utility::encodeTime(getTime()) << "\n";
	}
	
	// Detect a complete carrier (or exit if end of tape reached)
	bool end_of_carrier_detected = false;
	double t_stable_carrier = -1;
	int lost_carrier_cycles = 0;
	while (!end_of_carrier_detected) {

		// Reset dummy byte detection if carrier is lost
		if (carrier_half_cycle_count == 0) {
			t_dummy_byte_start = -1;
			detected_dummy_byte = false;
			t_dummy_byte = -1;
		}

		// Record time when the carrier cycles starts to come
		if (t_stable_carrier < 0 && carrier_half_cycle_count > carrier_duration_threshold_half_cycles && lost_carrier_cycles == 0) {
			t_stable_carrier = getTime() - (double) carrier_half_cycle_count / (2 * carrierFreq());
			waitingTime = t_stable_carrier - t_wait_start;
		} 
		else if (carrier_half_cycle_count < carrier_duration_threshold_half_cycles || lost_carrier_cycles > 0) {
			t_stable_carrier = -1;
			waitingTime = -1;
			preludeCycles = -1;
		}

		// Get next 1/2 cycle
		double t12_start = getTime();
		checkpoint();
		if (!mCycleDecoder.advanceHalfCycle()) {
			regretCheckpoint();
			return false; // End of tape reached if not 1/2 cycle could be detected => STOP
		}

		double half_cycle_duration = getTime() - t12_start;

		bool check_for_dummy_byte = false;

		// Increase carrier count for F2 cycle and decrease it for F1/undefined type of 1/2 cycles
		if (mCycleDecoder.lastHalfCycleFrequency() == Frequency::F2) {
			carrier_half_cycle_count++;
			half_cycle_duration_acc += half_cycle_duration;
			encountered_carrier_half_cycles++;
			lost_carrier_cycles = 0;
		} 
		else {
			lost_carrier_cycles += (int)round(half_cycle_duration * carrierFreq() * 2);
			carrier_half_cycle_count -= (int)round(half_cycle_duration * carrierFreq() * 2); // decrease = time passed in F2 units
		}

		if (carrier_half_cycle_count >= min_half_carrier_cycles && afterCarrierType == GAP_FOLLOWS && mCycleDecoder.lastHalfCycleFrequency() == Frequency::UndefinedFrequency)
			end_of_carrier_detected = true;

		// Check for a dummy byte/premable byte/data block byte (the latter for Acorn Atom after micro tone)
		if (mCycleDecoder.lastHalfCycleFrequency() == Frequency::F1 && (afterCarrierType & START_BIT_FOLLOWS)) {
			double t_byte_start = getTime();
			Byte byte;
			double max_byte_duration = 1.5 * mBitTiming.F2CyclesPerByte / carrierFreq();
			checkpoint();
			if (readByte(byte, false) && getTime() - t_byte_start < max_byte_duration) {
				if (
					(afterCarrierType == DATA_FOLLOWS || byte == 0x2a) &&
					carrier_half_cycle_count >= carrier_duration_threshold_half_cycles
				) {
					// Preamble <=> start of header => stop
					rollback(); // rollback to before preamble
					rollback(); // rollback to before first F1 1/2 cycle
					end_of_carrier_detected = true;
					if (mDebugInfo.verbose && carrier_half_cycle_count < min_half_carrier_cycles) {
						cout << "Unexpected start of block header (before min carrier time has elapsed) at " << Utility::encodeTime(getTime()) << "\n";
					} if (mDebugInfo.verbose)
						cout << "Start of block header at " << Utility::encodeTime(getTime()) << "\n";
					// Make sure any preceeding F1 1/2 cycle is ignored for start bit detection later on
					// if the preamble detection happened to 'lock' on the second F1 1/2 cycle...
					mCycleDecoder.setLastHalfCycleFrequency(Frequency::F2);  
				}
				else if (!detected_dummy_byte && detectDummyByte &&  byte == 0xaa) {
					// Dummy byte <> 0xaa => save the detected timing
					preludeCycles = (int)round((t_byte_start - t_stable_carrier) * mCycleDecoder.carrierFreq());
					t_dummy_byte = getTime() - t_byte_start; // stop bits not included as not read for a byte
					int dummy_byte_half_cycles = (int)round(t_dummy_byte * carrierFreq() * 2);
					detected_dummy_byte = true;
					t_dummy_byte_start = t_byte_start;
					foundDummyByte = byte;
					if (mDebugInfo.verbose) {
						//cout << "Detected dummy byte 0xaa at " << Utility::encodeTime(getTime()) << "\n";
					}
					regretCheckpoint(); // remove checkpoint made before reading the dummy byte
					regretCheckpoint(); // remove checkpoint made before reading the initial F1 1/2 cycle
				}
				else {
					// Neither preamble nor dummy byte => Treat this as noise and continue search
					rollback(); // rollback to before attempt to read byte
					regretCheckpoint(); // remove checkpoint made before reading the initial F1 1/2 cycle
				}
			}
			else {
				// No byte successfyll read => Treat this as noise and continue search
				rollback(); // rollback to before attempt to read byte
				regretCheckpoint(); // remove checkpoint made before reading the initial F1 1/2 cycle
			}
		}
		else {
			// No F1 1/2 cycle => remove checkpoint an continue search
			regretCheckpoint();
		}
	
		// If a gap (or an extremely long 1/2 cycle) was detected, the carrier 1/2 count might have to be reset
		if (carrier_half_cycle_count < 0 || lost_carrier_cycles > 2)
			carrier_half_cycle_count = 0;
	}

	// Calculate carrier timing
	double duration = getTime() - t_stable_carrier;
	double prelude_duration = (double) preludeCycles / carrierFreq();
	if (preludeCycles > 0)
		postludecycles = (int) round((duration - prelude_duration - t_dummy_byte) * carrierFreq());
	else
		postludecycles = (int) round(duration * carrierFreq());

	double carrier_cycle_freq_av = 1 / (2 * half_cycle_duration_acc / encountered_carrier_half_cycles);
	double base_freq_av = carrier_cycle_freq_av / 2;

	// Update bit timing based on new measured carrier frequency
	if (mDebugInfo.verbose) {
		if (detected_dummy_byte)
			cout << "Found dummy byte 0x" << hex << (int)foundDummyByte << " at " << Utility::encodeTime(t_dummy_byte_start) << "\n";
		cout << "Carrier of duration " << duration << "s detected at " << Utility::encodeTime(getTime()) << "\n";
		cout << "Detected a carrier frequency of " << carrier_cycle_freq_av << " - updating bit timing and cycle decoder with this!\n";
		cout << "Next 1/2 cycle is " << _FREQUENCY(mCycleDecoder.lastHalfCycleFrequency()) << "\n";
	}
	mCycleDecoder.setCarrierFreq(carrier_cycle_freq_av);
	BitTiming updated_bit_timing(mCycleDecoder.getSampleFreq(), base_freq_av, mTapeTiming.baudRate, mTargetMachine);
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
	return mCycleDecoder.checkpoint();
}

// Roll back to a previously saved file position
bool WavTapeReader::rollback()
{
	return mCycleDecoder.rollback();
}

// Remove checkpoint (without rolling back)
bool WavTapeReader::regretCheckpoint()
{
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
// Detect a start bit by looking for exactly mStartBitCycles of F1 1/2 cycles
// 
// If restart is allowed, 1/2 cycles will be read until the first F1 1/2 cycle is detected.
// If restart is NOT allowed, then the first read 1/2 cycle is expected to be an F1 1/2 cycle (possibly also
// including a previously read F1 1/2 cycle).
//
bool WavTapeReader::getStartBit(bool restartAllowed)
{

	mDataSamples = 0.0;
	mBitNo = 0;

	
	int n_remaining_start_bit_half_cycles = mBitTiming.startBitCycles * 2;

	DEBUG_PRINT(
		getTime(), DBG, "Last 1/2 Cycle was of type %s, waiting for %d 1/2 cycles of F1 when restart is %sallowed\n",
		mCycleDecoder.getHalfCycle().info().c_str(), n_remaining_start_bit_half_cycles, (restartAllowed ? "" : "not ")
	);
	
	double t_start = getTime();
	double waiting_time = 0;
	if (restartAllowed) {
		// For start bit for which a first F1 1/2 cycle has NOT already been sampled
		// If the previous cycle was an F1 1/2 cycle, then it was part of a last LOW data bit of the
		// previous byte. In that case, advance until at least one F2 1/2 cycle is detected (that would
		// then by a 1/2 cycle belonging to the stop bit of the previous byte).
		while (mCycleDecoder.lastHalfCycleFrequency() != Frequency::F2) {
			if (!mCycleDecoder.advanceHalfCycle())
				return false; // End of samples
		}
	}

	// Advance to a first F1 1/2 cycle
	// (If restart was not allowed and one F1 1/2 cycle had already been sampled, then we will stop directly.)
	while (mCycleDecoder.lastHalfCycleFrequency() != Frequency::F1) {
		if (!mCycleDecoder.advanceHalfCycle())
			return false; // End of samples		
	}
	--n_remaining_start_bit_half_cycles;
	waiting_time = getTime() - t_start;


	// Now read remaining F1 1/2 cycles
	DEBUG_PRINT(getTime(), DBG, "Wait for a continuous sequence of %d F1 1/2 cycles\n", n_remaining_start_bit_half_cycles);
	if (mCycleDecoder.lastHalfCycleFrequency() == Frequency::F1) {
		while (n_remaining_start_bit_half_cycles > 0 && mCycleDecoder.lastHalfCycleFrequency() == Frequency::F1 &&
			mCycleDecoder.advanceHalfCycle()) {
			--n_remaining_start_bit_half_cycles;
		}
		if (n_remaining_start_bit_half_cycles != 0)
			return false;
		return true;
	}

	DEBUG_PRINT(
		getTime(), DBG,
		"start bit detected after waiting %d 1/2 F2 cycles before the first F1 cycle\n",
		(int)round(waiting_time / (mCycleDecoder.carrierFreq() * 2))
	);

	return true;
}

//
// Determine whether the 1/2 cycles detected for the duration of a data bit corresponds to a '0' or a '1' value
// 
// 1/2 Cycle sequence			Phase determination									Data bit determination	#transitions
// F2 +	2n x F1 +	F2			low first F1 => 180 degrees, high = 0 degrees		'0'						[2n-1,2n+1]
// F12 +	2n-1 x F1 +	F12		low first F1 => 90 degrees, high => 270 degrees		'0'						2n
// F1 +	4n x F2 +	F1			low first F2 => 180 degrees, high = 0 degrees		'1'						[4n-1,4n+1]
// F12 +	4n-1 x F2 +	F12		low first F2 => 90 degrees, high => 270 degrees		'1'						4n//
//
bool WavTapeReader::getDataBit(Bit& bit)
{
	int n_half_cycles;
	int n_bit_samples = (int) (round(mDataSamples + mBitTiming.dataBitSamples) - round(mDataSamples));
	mDataSamples += mBitTiming.dataBitSamples;
	mBitNo++;
	double t_start = getTime();

	if (mDebugInfo.tracing)
		DEBUG_PRINT(getTime(), DBG, "%f->%f:%d\n", mDataSamples - mBitTiming.dataBitSamples, mDataSamples, n_bit_samples);

	// Advance time corresponding to one bit and count the no of transitions (1/2 cycles)
	int min_half_cycle_duration, max_half_cycle_duration;
	if (!mCycleDecoder.countHalfCycles(n_bit_samples, n_half_cycles, min_half_cycle_duration, max_half_cycle_duration)) {
		if (mDebugInfo.tracing)
			DEBUG_PRINT(getTime(), ERR, "Unexpected end of samples when reading data bit%s\n", "");
		return false; // unexpected end of samples
	}

	// Decide whether the databit was a '0' or a '1' value based on the no of detected 1/2 cycles and the max 1/2 cycle duration
	if (
		n_half_cycles <= mBitTiming.dataBitHalfCycleBitThreshold && 
		mCycleDecoder.strictValidHalfCycleRange(F1, max_half_cycle_duration)
		)
		bit = LowBit;
	else {
		bit = HighBit;
	}

	if (mDebugInfo.verbose)
		DEBUG_PRINT(getTime(), DBG, "%d 1/2 cycles (of max duration %d) detected for data bit and therefore classified as a '%d'\n", n_half_cycles, max_half_cycle_duration, bit);

	return true;

}

bool WavTapeReader::getStopBit()
{
	// Get one cycle of high tone to make sure we're into stop bit before
	// searching for a start bit for the next byte

	double waiting_time;
	if (!mCycleDecoder.stopOnHalfCycles(Frequency::F2, 2, waiting_time)) {
		if (mDebugInfo.tracing)
			DEBUG_PRINT(getTime(), ERR, "Failed to detect stop bit%s\n", "");
		return false;
	}

	DEBUG_PRINT(getTime(), DBG, "Stop bit consumed with last 1/2 cycle being %s\n", _FREQUENCY(mCycleDecoder.lastHalfCycleFrequency()));

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