#include "SidechainFilter.h"
#include <algorithm>
#include <cmath>

SidechainFilter::SidechainFilter()
{
    // Initialise all filters with default coefficients so they are valid
    // even if process() is called before prepare().
    updateCoefficients();
}

void SidechainFilter::prepare(double sampleRate, int /*maxBlockSize*/)
{
    mSampleRate = sampleRate;

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        mHP[ch].reset();
        mLP[ch].reset();
        mTiltLS[ch].reset();
        mTiltHS[ch].reset();
    }

    updateCoefficients();
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
    mPendingHP.store(hz, std::memory_order_relaxed);
    mCoeffsDirty.store(true, std::memory_order_release);
}

void SidechainFilter::setLowPassFreq(float hz)
{
    mPendingLP.store(hz, std::memory_order_relaxed);
    mCoeffsDirty.store(true, std::memory_order_release);
}

void SidechainFilter::setTilt(float dB)
{
    mPendingTilt.store(dB, std::memory_order_relaxed);
    mCoeffsDirty.store(true, std::memory_order_release);
}

void SidechainFilter::updateCoefficients()
{
    // High-pass: 2nd-order Butterworth
    auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
        mSampleRate, static_cast<float>(mHPFreq));

    // Low-pass: 2nd-order Butterworth
    auto lpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(
        mSampleRate, static_cast<float>(mLPFreq));

    // Tilt EQ: split ±tiltDb/2 dB across low shelf and high shelf at 1 kHz.
    // Positive tilt: lows cut, highs boosted (tilts spectrum upward).
    const float lowGain  = std::pow(10.0f, -mTiltDb * 0.5f / 20.0f);
    const float highGain = std::pow(10.0f,  mTiltDb * 0.5f / 20.0f);

    auto lsCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        mSampleRate, 1000.0f, 0.707f, lowGain);

    auto hsCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        mSampleRate, 1000.0f, 0.707f, highGain);

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        mHP[ch].coefficients     = hpCoeffs;
        mLP[ch].coefficients     = lpCoeffs;
        mTiltLS[ch].coefficients = lsCoeffs;
        mTiltHS[ch].coefficients = hsCoeffs;
    }
}
