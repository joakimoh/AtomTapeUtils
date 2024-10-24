#include <iostream>
#include <cmath>


#include "Filter.h" 
#include "../shared/Logging.h"
#include "../shared/Utility.h"
#include "../shared/WaveSampleTypes.h"
#include <functional>

#include "ArgParser.h"

Filter::Filter(int sampleFreq, ArgParser argParser)
{
    mFS = sampleFreq;
    mTS = 1 / mFS;
    mAveragePoints = argParser.nAveragingSamples;
    derivativeThreshold = argParser.derivativeThreshold;
    mMaxSampleAmplitude = argParser.sinusAmplitude;
    minPeakDistanceSamples = (int) round(argParser.minPeakDistance * mFS / F2_FREQ);
    saturationLevelLow = (int) round(argParser.saturationLevelLow * SAMPLE_LOW_MIN);
    saturationLevelHigh = (int)round(argParser.saturationLevelHigh * SAMPLE_HIGH_MAX);

}

bool Filter::averageFilter(Samples& samples, Samples& filtered_samples)
{
    const int n = mAveragePoints;
    int sz = (int) samples.size();
    for (int p = 0; p < sz; p++) {
        if (p <= n || sz - p <= n)
            filtered_samples[p] = samples[p];
        else {
            int sum = 0;
            for (int i = -n; i <= n; i++)
                sum += samples[p+i];
            Sample av = sum / (2 * n + 1);
            filtered_samples[p]= av;
        }
    }
    
    return true;
}

bool Filter::normaliseFilter(Samples& samples, ExtremumSamples& outSamples, int& nOutSamples)
{

    int sample_pos = 0;
    int sample_sz = (int) samples.size();
    Extremum extremum = PLATEAU;
    int extremum_pos = 0;
    nOutSamples = 0;

    while (sample_pos < sample_sz) {

        int new_extremum_pos;
        Extremum new_extremum;

        // Search for extremum (local min, local max, plateau)
        if (!find_extreme(sample_pos, samples, extremum, new_extremum, new_extremum_pos)) {
            // End of samples - no more extremes found
            return true;
        }
        else {
            
            // Extremum found
             extremum = new_extremum;
            ExtremumSample extremum_sample = { new_extremum, new_extremum_pos};
            outSamples[nOutSamples++] = extremum_sample;
           
            if (sample_pos < sample_sz)
                sample_pos++;
        }
    }

    return true;
}

/*
* 
* Calculate the derivate based on two samples s(t+T) and s(t) only.
* 
* For a sinuse curve of frequency 2400 and a sample frequency of 44.1 kHz the absolute value of the derivative [1/rad] is
* ((s_T - s_0) / max_amplitude) / (2 * pi * 2400  / 44 100) = [2.92 / max_amplitude, 2.92] for a non-zero derivative
* ([5.85 / max_amplitude, 5.85] for 1200 Hz). The derivative of a sinus curve shall however be [0, 1].
* 
* (s_T - s_0) should be 2 * pi * max_amplitude * (2400 / 44 100) * der = 0.34 * der = [0.34, 0.34 * max_amplitude] for a
* non-zero derivative of a sinus curve. ([0.17, 0.17 * max_amplitude] for 1200 Hz)
* 
* So if the derivative is larger than 0.34 * 32768 = 11 141 then it cannot be a 1200/2400 Hz sinus curve value but noise.
* Don't know how to make use of this to improve the filtering right now...
* 
*/
bool Filter::derivative(int pos, Samples &samples, int nSamples, double & d)
{


    if (pos < nSamples - 1) {
        Sample d0 = samples[pos];
        Sample d1 = samples[pos+ 1];
        if (d0 < saturationLevelLow)
            d0 = saturationLevelLow;
        if (d0 > saturationLevelHigh)
            d0 = saturationLevelHigh;
        if (d1 < saturationLevelLow)
            d1 = saturationLevelLow;
        if (d1 > saturationLevelHigh)
            d1 = saturationLevelHigh;
        d = d1 - d0;
        //cout << "Derivative = " << d << " (" << slope(d) << ") at value " << (int)*pos << " and position " << (int)(pos - samples.begin()) << "...\n";
    }
    else
        return false;
       

    return true;
}

bool Filter::find_extreme(int &pos, Samples& samples, Extremum prevExtremum, Extremum& newExtremum, int& nexExtremumPos)
{
    double d;
    int p = pos;
    int p1, p2;
    int sz = (int) samples.size();
    

    int prev_slope;

    // Determine starting slope

    if (prevExtremum == LOCAL_MIN || prevExtremum == START_POS_SLOPE)

        prev_slope = +1;

    else if (prevExtremum == LOCAL_MAX || prevExtremum == START_NEG_SLOPE)

        prev_slope = -1;

    else { // prevExtremum == PLATEAU

        // Wait for a non-zero derivative
        while (derivative(p, samples, sz, d) && slope(d) == 0) {
            p++;
        }
        if (p == sz)
            return false;

        if (slope(d) < 0)
            newExtremum = START_NEG_SLOPE; 
        else
            newExtremum = START_POS_SLOPE;
        nexExtremumPos = p;
        pos = p;
        return true;
     }

  

    // Wait for a different slope while ignoring all intermediate zero slopes
    
    bool different_slope = false;
    bool first_round = true;
    int count = 0;
    const int PLATEAU_DURATION = 100;
    while (!different_slope) {

        // Wait for a different slope (including a zero slope)
        while (derivative(p, samples, sz, d) && slope(d) == prev_slope)
            p++;
        if (p < sz) {
            if (slope(d) != 0) {
                // Non zero slope => done
                different_slope = true;
                count = 0;
            }
            else if (count < PLATEAU_DURATION) {
                // zero slope - just skip it as long as it is short
                p++;
                count++;
            }
            else {
                // Long duration plateau <=> carrier off
                newExtremum = PLATEAU;
                nexExtremumPos = pos;
                pos = p;
                return true;
            }
        }
        else
            return false;
        
        // Remember start of change position
        if (first_round) {
            p1 = p;
            first_round = false;
        }

    }
 
    // Remember end of change position
    p2 = p;

    // Calculate middle position of peak
    nexExtremumPos = p1 + (p2 - p1) / 2;
    pos = p;

    // Check if it was a local minimum
    if  (prev_slope < 0 && slope(d) > 0) {
        // Local minimum
        newExtremum = LOCAL_MIN;
        return true;
    }

    // Check if it was a local maximum
    else if  (prev_slope > 0 && slope(d) < 0) {
        // Local maximum
        newExtremum = LOCAL_MAX;
        return true;
    }

    // Can only end here if out of samples...
    else { 
        // End of samples
        return false;
    }

}

int Filter::slope(double i)
{
    if (i >= -derivativeThreshold && i <= derivativeThreshold)
        return 0;
    else if (i > derivativeThreshold)
        return +1;
    else
        return -1;
}

void Filter::plotDebug(int debugLevel, ExtremumSample& sample, ExtremumSample& prevSample, int extremumIndex, ExtremumSamples& samples)
{
    plotDebug(debugLevel, "", prevSample, sample, extremumIndex, samples);
}

void Filter::plotDebug(int debugLevel, string text, ExtremumSample& sample, int extremumIndex, ExtremumSamples& samples)
{
    string t = Utility::encodeTime((double)sample.pos) + " (" + to_string(sample.pos) + ")";
    string e = _EXTREMUM(sample.extremum);
    string p = to_string(extremumIndex);
 
    if (debugLevel == DBG)
        return;
 
    DBG_PRINT(debugLevel, "%s %s at %s (%s)\n", text.c_str(), e.c_str(), t.c_str(), p.c_str());
}

void Filter::plotDebug(int debugLevel, string text, ExtremumSample &sample, ExtremumSample & prevSample, int extremumIndex, ExtremumSamples& samples)

{
    string t = Utility::encodeTime((double) sample.pos / mFS) + " (" + to_string(sample.pos) + ")";
    string pt = Utility::encodeTime((double)prevSample.pos / mFS) + " (" + to_string(prevSample.pos) + ")";
    string e = _EXTREMUM(sample.extremum);
    string pe = _EXTREMUM(prevSample.extremum);
    string p = to_string(extremumIndex);

    if (debugLevel == DBG)
        return;

    DBG_PRINT(debugLevel, "%s %s (%s) at %s after %s at %s\n", text.c_str(),
        e.c_str(), t.c_str(), p.c_str(),
        pe.c_str(), pt.c_str()
    );
}

bool Filter::plotFromExtremums(FilterType filterType, int nExtremums, ExtremumSamples& extremums, Samples& inSamples, Samples& outSamples, int nSamples)
{
    if (filterType == SCALE)
        return plotFromExtremums(
            [this](double phase1, double phase2, int nSamples, Samples& inSamples, Samples& outSamples, int& sampleIndex)
            { return scaleSegment(phase1, phase2, nSamples, inSamples, outSamples, sampleIndex); },
            nExtremums, extremums, inSamples, outSamples, nSamples
        );
    else
        return plotFromExtremums(
            [this](double phase1, double phase2, int nSamples, Samples& inSamples, Samples& outSamples, int& sampleIndex)
            { return crCurveSegment(phase1, phase2, nSamples, inSamples, outSamples, sampleIndex); },
            nExtremums, extremums, inSamples, outSamples, nSamples
        );
}

bool Filter::plotFromExtremums(
        function<bool (double, double, int, Samples&, Samples&, int& )> plotFunction,
    int nExtremums, ExtremumSamples& extremums, Samples &inSamples, Samples& outSamples, int nSamples)
{
    int extremum_index = 0;
    int sample_index = 0;
    ExtremumSample prev_extremum_sample = { PLATEAU, 0 };

    int prev_extremum_index = extremum_index;
    

    while (extremum_index < nExtremums) {

        ExtremumSample extremum_sample = extremums[extremum_index];
        mExtremumSample = extremum_sample;
        int n_samples_between_extremums = extremum_sample.pos - prev_extremum_sample.pos;

        switch (extremum_sample.extremum) {
        case LOCAL_MIN:
            if (prev_extremum_sample.extremum == START_NEG_SLOPE) {
                //plotDebug(DBG, extremum_sample, prev_extremum_sample, extremum_index, extremums);
                if (!plotFunction(180, 270, n_samples_between_extremums, inSamples, outSamples, sample_index))
                    return false;
            } else if (prev_extremum_sample.extremum == LOCAL_MAX) {
                if (extremum_sample.pos - prev_extremum_sample.pos > minPeakDistanceSamples) {
                    //plotDebug(DBG, extremum_sample, prev_extremum_sample, extremum_index, extremums);
                    if (!plotFunction(90, 270, n_samples_between_extremums, inSamples, outSamples, sample_index))
                        return false;
                }
                else {

                    bool found_max = false;

                    //plotDebug(DBG, "False ", extremum_sample, prev_extremum_sample, extremum_index, extremums);
                    // Noise and not a new minimum => advance to next LOCAL_MIN/START_NEG_SLOPE/PLATEAU (or end of samples)
                    while (extremum_index < nExtremums && extremum_sample.extremum == Extremum::LOCAL_MIN) {
                        //plotDebug(DBG, "Skipping MIN", extremum_sample, extremum_index, extremums);
                        extremum_sample = extremums[extremum_index++];
                    }            
                    while (extremum_index < nExtremums && extremum_sample.extremum == Extremum::LOCAL_MAX) {
                        found_max = true;
                        //plotDebug(DBG, "Skipping MAX", extremum_sample, extremum_index, extremums);
                        extremum_sample = extremums[extremum_index++];
                     }

                    // Now the next sample is expected to be a new (hopefully true) MIN but can also be a PLATEAU...
                    
                    // Roll back one sample as iterator incremented at the end of the loop
                    if (extremum_index < nExtremums && found_max) {
                        //plotDebug(DBG, "Skipped to sample", extremum_sample, extremum_index, extremums);
                        extremum_sample = extremums[--extremum_index];
                        //plotDebug(DBG, "Corrected sample iterator pointing at sample", extremum_sample, extremum_index, extremums);
                        
                    }

                    if (extremum_index < nExtremums && !found_max) { // PLATEAU => plot as for MIN->PLATEAU below
                        if (extremum_sample.extremum == Extremum::PLATEAU) {
                            //plotDebug(DBG, extremum_sample, prev_extremum_sample, extremum_index, extremums);
                            n_samples_between_extremums = extremum_sample.pos - prev_extremum_sample.pos;
                            if (!plotFunction(-90, 0, n_samples_between_extremums, inSamples, outSamples, sample_index))
                                return false;
                        }
                        else {
                            plotDebug(ERR, "Unexpected transition", extremum_sample, prev_extremum_sample, extremum_index, extremums);
                            return false;
                        }
                        extremum_sample = extremum_sample;
                    }
                    else
                        extremum_sample = prev_extremum_sample;// make sure the previous valid extremum is the reference in the next round
                }
                
            } else {
                // Error as this should never happen...
                plotDebug(ERR, "Unexpected transition", extremum_sample, prev_extremum_sample, extremum_index, extremums);
                return false;
            }
          
            break;

        case LOCAL_MAX:
            if (prev_extremum_sample.extremum == START_POS_SLOPE) {
                //plotDebug(DBG, extremum_sample, prev_extremum_sample, extremum_index, extremums);
                if (!plotFunction(0, 90, n_samples_between_extremums, inSamples, outSamples, sample_index))
                    return false;
            }
            else if (prev_extremum_sample.extremum == LOCAL_MIN) {
                if (extremum_sample.pos - prev_extremum_sample.pos > minPeakDistanceSamples) {
                    //plotDebug(DBG, extremum_sample, prev_extremum_sample, extremum_index, extremums);
                    if (!plotFunction(-90, 90, n_samples_between_extremums, inSamples, outSamples, sample_index))
                        return false;
                }
                else {

                    //plotDebug(DBG, "False", extremum_sample, prev_extremum_sample, extremum_index, extremums);

                    bool found_min = false;

                    // Noise and not a new maximum => advance to next LOCAL_MAX/PLATEAU (or end of samples)
                    while (extremum_index < nExtremums && extremum_sample.extremum == Extremum::LOCAL_MAX) {
                        //plotDebug(DBG, "Skipping MAX", extremum_sample, extremum_index, extremums);
                        extremum_sample = extremums[extremum_index++];
                    }
                    while (extremum_index < nExtremums && extremum_sample.extremum == Extremum::LOCAL_MIN ) {
                        found_min = true;
                        //plotDebug(DBG, "Skipping MIN", extremum_sample, extremum_index, extremums);
                        extremum_sample = extremums[extremum_index++];
                    }

                    // Now the next sample is expected to be a new (hopefully true) MAX but can also be a PLATEAU...
                    
                    // Roll back one sample as iterator incremented at the end of the loop
                    if (extremum_index < nExtremums && found_min) {
                        //plotDebug(DBG, "Skipped to sample", extremum_sample, extremum_index, extremums);
                        extremum_sample = extremums[--extremum_index];
                        //plotDebug(DBG, "Corrected sample iterator pointing at sample", extremum_sample, extremum_index, extremums);
                        
                    }

                    if (extremum_index < nExtremums && !found_min) { // PLATEAU => plot as for MAX->PLATEAU below
                        if (extremum_sample.extremum == Extremum::PLATEAU) {
                            //plotDebug(DBG, extremum_sample, prev_extremum_sample, extremum_index, extremums);
                            n_samples_between_extremums = extremum_sample.pos - prev_extremum_sample.pos;
                            if (!plotFunction(90, 180, n_samples_between_extremums, inSamples, outSamples, sample_index))
                                return false;
                        }
                        else {
                            //plotDebug(ERR, "Unexpected transition", extremum_sample, prev_extremum_sample, extremum_index, extremums);
                            return false;
                        }
                    } 
                    else
                        extremum_sample = prev_extremum_sample;// make sure the previous valid extremum is the reference in the next round
                }
            }
            else {
                // Error as this should never happen...
                //plotDebug(ERR, "Unexpected transition", extremum_sample, prev_extremum_sample, extremum_index, extremums);
                return false;
            }

            break;

        case PLATEAU:
            if (prev_extremum_sample.extremum == LOCAL_MIN) {
                //plotDebug(DBG, extremum_sample, prev_extremum_sample, extremum_index, extremums);
                if (!plotFunction(-90, 0, n_samples_between_extremums, inSamples, outSamples, sample_index))
                    return false;
            }
            else if (prev_extremum_sample.extremum == LOCAL_MAX) {
                //plotDebug(DBG, extremum_sample, prev_extremum_sample, extremum_index, extremums);
                if (!plotFunction(90, 180, n_samples_between_extremums, inSamples, outSamples, sample_index))
                    return false;
            }
            else if (prev_extremum_sample.extremum == START_NEG_SLOPE || prev_extremum_sample.extremum == START_POS_SLOPE) {
                //plotDebug(DBG, extremum_sample, prev_extremum_sample, extremum_index, extremums);
                // PLATEAU => START_NEG/POS_SLOPE => PLATEAU
                // This is a slope that never resulted in any local extremum
                for (int s = 0; s < n_samples_between_extremums; s++)
                    outSamples[sample_index++] = 0;
            }
            else {
                // Error as this should never happen...
                plotDebug(ERR, "Unexpected transition", extremum_sample, prev_extremum_sample, extremum_index, extremums);
                return false;
            }

            break;

        case START_NEG_SLOPE:
        case START_POS_SLOPE:
            if (prev_extremum_sample.extremum == PLATEAU) {
                //plotDebug(DBG, extremum_sample, prev_extremum_sample, extremum_index, extremums);
                for (int s = 0; s < n_samples_between_extremums; s++)
                    outSamples[sample_index++] = 0;

            }
            else {
                // Error as this should never happen...
                plotDebug(ERR, "Unexpected transition", extremum_sample, prev_extremum_sample, extremum_index, extremums);
                return false;
            }

            break;
        
        default: // should never happen
            plotDebug(ERR, "Unexpected transition", extremum_sample, prev_extremum_sample, extremum_index, extremums);
            return false;
        }


        if (extremum_index < nExtremums)
            extremum_index++;
  
        prev_extremum_sample = extremum_sample;
        mPrevExtremumSample = prev_extremum_sample;

        if (extremum_index == prev_extremum_index) {
            plotDebug(ERR, "Extremum iterator stuck at ", extremum_sample, prev_extremum_sample, extremum_index, extremums);
            break;
        }

        prev_extremum_index = extremum_index;
    }


    // Add dummy samples from the last extremum until end of samples
    int pos = prev_extremum_sample.pos;


    while (pos < nSamples)
        outSamples[pos++] = 0;


    return true;
}

bool Filter::crCurveSegment(double phase1, double phase2, int nSamples, Samples& inSamples, Samples& outShapes, int& sampleIndex)
{    
    const double PI = 3.14159265358979323846;
    double f = PI * (phase2 - phase1) / (180 * nSamples);
    double o = phase1 * PI / 180;
    for (int s = 0; s < nSamples; s++) {
        Sample y = (Sample) round(sin(o + s*f) * mMaxSampleAmplitude);
        outShapes[sampleIndex++] = y;
    }

    return true;
}

bool Filter::scaleSegment(
    double phase1, double phase2, int nSamples, Samples& inSamples, Samples& outShapes, int& sampleIndex
)
{
    if (sampleIndex + nSamples >= inSamples.size())
        return false;

    const double max_amplitude = (double) mMaxSampleAmplitude;
    double amplitude_change;
    double initial_amplitude;
    if (phase1 == 0 && phase2 == 90) {
        amplitude_change = max_amplitude;
        initial_amplitude = 0;
    } 
    else if (phase1 == 90 && phase2 == 180) {
        amplitude_change = -max_amplitude;
        initial_amplitude = max_amplitude;
    }
    else if (phase1 == 180 && phase2 == 270) {
        amplitude_change = -max_amplitude;
        initial_amplitude = 0;
    }
    else if (phase1 == -90 && phase2 == 0) {
        amplitude_change = max_amplitude;
        initial_amplitude = -max_amplitude;
    }
    else if (phase1 == 90 && phase2 == 270) {
        amplitude_change = -2 * max_amplitude;
        initial_amplitude = max_amplitude;
    }
    else if (phase1 == -90 && phase2 == 90) {
        amplitude_change = 2 * max_amplitude;
        initial_amplitude = -max_amplitude;
    }
    else
        return false;

    double e1 = (double) inSamples[sampleIndex]; // first extreme (either amplitude_change Min or amplitude_change Max)
    double e2 = (double) inSamples[sampleIndex + nSamples - 1]; // second extreme (either amplitude_change Max or amplitude_change Min)
    double ed = e2 - e1; // distance between extremes (peak-to-peak)
    for (int s = 0; s < nSamples; s++) {
        double v = (double) inSamples[sampleIndex];
        Sample y = (Sample) round((v - e1) / ed * amplitude_change + initial_amplitude);
        outShapes[sampleIndex++] = y;
    }

    return true;
}