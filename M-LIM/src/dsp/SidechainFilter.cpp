#include "SidechainFilter.h"
#include "DspUtil.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdint>

SidechainFilter::SidechainFilter()
{
    // Initialise all filters with valid 2nd-order coefficients so they are
    // safe to use even if process() is called before prepare().
    prepare(44100.0, 1);
}

void SidechainFilter::prepare(double sampleRate, int /*maxBlockSize*/)
{
    mSampleRate = sampleRate;

    // Compute initial tilt gains for the shelf filters.
    const float lowGain  = std::pow(10.0f, -mTiltDb * 0.5f / 20.0f);
    const float highGain = std::pow(10.0f,  mTiltDb * 0.5f / 20.0f);

    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        // Allocate coefficient objects via factory methods — allocation is
        // acceptable here because prepare() runs on a non-RT thread.
        mHPCoeffs[ch] = Coeffs::makeHighPass  (mSampleRate, mHPFreq);
        mLPCoeffs[ch] = Coeffs::makeLowPass   (mSampleRate, mLPFreq);
        mLSCoeffs[ch] = Coeffs::makeLowShelf  (mSampleRate, 1000.0f, 0.707f, lowGain);
        mHSCoeffs[ch] = Coeffs::makeHighShelf (mSampleRate, 1000.0f, 0.707f, highGain);

        // Point each filter at its pre-allocated coefficient object so that
        // updateCoefficients() can overwrite the values in-place later.
        mHP[ch].coefficients     = mHPCoeffs[ch];
        mLP[ch].coefficients     = mLPCoeffs[ch];
        mTiltLS[ch].coefficients = mLSCoeffs[ch];
        mTiltHS[ch].coefficients = mHSCoeffs[ch];

        // Reset filter state; reset() uses the current coefficients to
        // determine the filter order, so it must run after the assignment.
        mHP[ch].reset();
        mLP[ch].reset();
        mTiltLS[ch].reset();
        mTiltHS[ch].reset();
    }
}

void SidechainFilter::process(juce::AudioBuffer<float>& buffer)
{
    // Apply any pending parameter changes before processing.
    // mCoeffsDirty is written by setters on other threads; we load it here
    // on the audio thread. Using exchange so we don't miss a change that
    // arrives between the load and the clear.
    if (mCoeffsDirty.exchange(false, std::memory_order_acq_rel))
    {
        mHPFreq = mPendingHP.load(std::memory_order_relaxed);
        mLPFreq = mPendingLP.load(std::memory_order_relaxed);
        mTiltDb = mPendingTilt.load(std::memory_order_relaxed);
        updateCoefficients();
    }

    juce::ScopedNoDenormals noDenormals;
    const int numChannels = std::min(buffer.getNumChannels(), kMaxChannels);
    const int numSamples  = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            float s = data[i];
            s = mHP[ch].processSample(s);
            s = mLP[ch].processSample(s);
            s = mTiltLS[ch].processSample(s);
            s = mTiltHS[ch].processSample(s);
            data[i] = s;
        }
    }
}


void SidechainFilter::setHighPassFreq(float hz)
{
    hz = std::clamp(hz, 20.0f, 2000.0f);
    if (floatBitsEqual (mPendingHP.load(std::memory_order_relaxed), hz))
        return;
    mPendingHP.store(hz, std::memory_order_relaxed);
    mCoeffsDirty.store(true, std::memory_order_release);
}

void SidechainFilter::setLowPassFreq(float hz)
{
    hz = std::clamp(hz, 2000.0f, 20000.0f);
    if (floatBitsEqual (mPendingLP.load(std::memory_order_relaxed), hz))
        return;
    mPendingLP.store(hz, std::memory_order_relaxed);
    mCoeffsDirty.store(true, std::memory_order_release);
}

void SidechainFilter::setTilt(float dB)
{
    dB = std::clamp(dB, -6.0f, 6.0f);
    if (floatBitsEqual (mPendingTilt.load(std::memory_order_relaxed), dB))
        return;
    mPendingTilt.store(dB, std::memory_order_relaxed);
    mCoeffsDirty.store(true, std::memory_order_release);
}

// ---------------------------------------------------------------------------
// updateCoefficients — called only from the audio thread (inside process()).
//
// Computes all biquad coefficients inline using the same bilinear-transform
// formulas as JUCE's ArrayCoefficients factory methods, then writes the five
// normalised values (b0, b1, b2, a1, a2) directly into each filter's
// pre-allocated Coefficients array via getRawCoefficients().
//
// No heap allocation occurs here: all intermediate values are stack-local,
// and the destination arrays were pre-allocated once in prepare().
// ---------------------------------------------------------------------------
void SidechainFilter::updateCoefficients()
{
    // -----------------------------------------------------------------------
    // High-pass: 2nd-order Butterworth
    // n = tan(pi * fc / fs),  invQ = sqrt(2)  (default Butterworth Q = 1/sqrt(2))
    // c1 = 1 / (1 + invQ*n + n^2)
    // b = [c1, -2*c1, c1]   a = [2*c1*(n^2-1), c1*(1 - invQ*n + n^2)]
    // -----------------------------------------------------------------------
    {
        const float pi      = juce::MathConstants<float>::pi;
        const float sqrt2   = juce::MathConstants<float>::sqrt2;
        const float n       = std::tan(pi * mHPFreq / static_cast<float>(mSampleRate));
        const float nSq     = n * n;
        const float c1      = 1.0f / (1.0f + sqrt2 * n + nSq);

        const float hp_b0   =  c1;
        const float hp_b1   = -2.0f * c1;
        const float hp_b2   =  c1;
        const float hp_a1   =  2.0f * c1 * (nSq - 1.0f);
        const float hp_a2   =  c1 * (1.0f - sqrt2 * n + nSq);

        for (int ch = 0; ch < kMaxChannels; ++ch)
        {
            float* raw = mHP[ch].coefficients->getRawCoefficients();
            raw[0] = hp_b0;
            raw[1] = hp_b1;
            raw[2] = hp_b2;
            raw[3] = hp_a1;
            raw[4] = hp_a2;
        }
    }

    // -----------------------------------------------------------------------
    // Low-pass: 2nd-order Butterworth
    // n = cot(pi * fc / fs) = 1/tan(pi * fc / fs),  invQ = sqrt(2)
    // c1 = 1 / (1 + invQ*n + n^2)
    // b = [c1, 2*c1, c1]   a = [2*c1*(1 - n^2), c1*(1 - invQ*n + n^2)]
    // -----------------------------------------------------------------------
    {
        const float pi      = juce::MathConstants<float>::pi;
        const float sqrt2   = juce::MathConstants<float>::sqrt2;
        const float n       = 1.0f / std::tan(pi * mLPFreq / static_cast<float>(mSampleRate));
        const float nSq     = n * n;
        const float c1      = 1.0f / (1.0f + sqrt2 * n + nSq);

        const float lp_b0   =  c1;
        const float lp_b1   =  2.0f * c1;
        const float lp_b2   =  c1;
        const float lp_a1   =  2.0f * c1 * (1.0f - nSq);
        const float lp_a2   =  c1 * (1.0f - sqrt2 * n + nSq);

        for (int ch = 0; ch < kMaxChannels; ++ch)
        {
            float* raw = mLP[ch].coefficients->getRawCoefficients();
            raw[0] = lp_b0;
            raw[1] = lp_b1;
            raw[2] = lp_b2;
            raw[3] = lp_a1;
            raw[4] = lp_a2;
        }
    }

    // -----------------------------------------------------------------------
    // Tilt EQ: low shelf + high shelf pivoting at 1 kHz.
    // Positive tilt dB => lows attenuated, highs boosted.
    //
    // Low-shelf gain  = 10^(-tilt * 0.5 / 20)
    // High-shelf gain = 10^(+tilt * 0.5 / 20)
    //
    // Shelf biquad coefficients (Audio EQ Cookbook, Q = 0.707):
    //   A     = sqrt(gainLinear)
    //   omega = 2*pi*f0 / fs
    //   beta  = sin(omega) * sqrt(A) / Q
    //
    // Low-shelf (raw 6-coeff before normalisation):
    //   b0 = A*(aplus1 - aminus1*cos + beta)
    //   b1 = 2A*(aminus1 - aplus1*cos)
    //   b2 = A*(aplus1 - aminus1*cos - beta)
    //   a0 = aplus1 + aminus1*cos + beta
    //   a1 = -2*(aminus1 + aplus1*cos)
    //   a2 = aplus1 + aminus1*cos - beta
    //
    // High-shelf flips the sign of the aminus1*cos terms and swaps a1 sign.
    // -----------------------------------------------------------------------
    {
        const float lowGain  = std::pow(10.0f, -mTiltDb * 0.5f / 20.0f);
        const float highGain = std::pow(10.0f,  mTiltDb * 0.5f / 20.0f);

        const float twoPi    = juce::MathConstants<float>::twoPi;
        const float omega    = twoPi * 1000.0f / static_cast<float>(mSampleRate);
        const float sinOmega = std::sin(omega);
        const float cosOmega = std::cos(omega);
        constexpr float kQ   = 0.707f;

        // --- Low shelf ---
        {
            const float A       = std::sqrt(lowGain);
            const float am1     = A - 1.0f;
            const float ap1     = A + 1.0f;
            const float beta    = sinOmega * std::sqrt(A) / kQ;
            const float am1c    = am1 * cosOmega;

            const float raw_b0  = A * (ap1 - am1c + beta);
            const float raw_b1  = A * 2.0f * (am1 - ap1 * cosOmega);
            const float raw_b2  = A * (ap1 - am1c - beta);
            const float raw_a0  = ap1 + am1c + beta;
            const float raw_a1  = -2.0f * (am1 + ap1 * cosOmega);
            const float raw_a2  = ap1 + am1c - beta;

            const float a0inv   = (raw_a0 != 0.0f) ? 1.0f / raw_a0 : 0.0f;
            const float ls_b0   = raw_b0 * a0inv;
            const float ls_b1   = raw_b1 * a0inv;
            const float ls_b2   = raw_b2 * a0inv;
            const float ls_a1   = raw_a1 * a0inv;
            const float ls_a2   = raw_a2 * a0inv;

            for (int ch = 0; ch < kMaxChannels; ++ch)
            {
                float* raw = mTiltLS[ch].coefficients->getRawCoefficients();
                raw[0] = ls_b0;
                raw[1] = ls_b1;
                raw[2] = ls_b2;
                raw[3] = ls_a1;
                raw[4] = ls_a2;
            }
        }

        // --- High shelf ---
        {
            const float A       = std::sqrt(highGain);
            const float am1     = A - 1.0f;
            const float ap1     = A + 1.0f;
            const float beta    = sinOmega * std::sqrt(A) / kQ;
            const float am1c    = am1 * cosOmega;

            const float raw_b0  =  A * (ap1 + am1c + beta);
            const float raw_b1  =  A * -2.0f * (am1 + ap1 * cosOmega);
            const float raw_b2  =  A * (ap1 + am1c - beta);
            const float raw_a0  =  ap1 - am1c + beta;
            const float raw_a1  =  2.0f * (am1 - ap1 * cosOmega);
            const float raw_a2  =  ap1 - am1c - beta;

            const float a0inv   = (raw_a0 != 0.0f) ? 1.0f / raw_a0 : 0.0f;
            const float hs_b0   = raw_b0 * a0inv;
            const float hs_b1   = raw_b1 * a0inv;
            const float hs_b2   = raw_b2 * a0inv;
            const float hs_a1   = raw_a1 * a0inv;
            const float hs_a2   = raw_a2 * a0inv;

            for (int ch = 0; ch < kMaxChannels; ++ch)
            {
                float* raw = mTiltHS[ch].coefficients->getRawCoefficients();
                raw[0] = hs_b0;
                raw[1] = hs_b1;
                raw[2] = hs_b2;
                raw[3] = hs_a1;
                raw[4] = hs_a2;
            }
        }
    }

    // Reset delay elements for all filters to avoid transient spikes caused
    // by stale state accumulated under the old coefficients. reset() is O(1)
    // and RT-safe (no allocation).
    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        mHP[ch].reset();
        mLP[ch].reset();
        mTiltLS[ch].reset();
        mTiltHS[ch].reset();
    }
}
