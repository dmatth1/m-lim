#include "Oversampler.h"

void Oversampler::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    mSampleRate   = sampleRate;
    mMaxBlockSize = maxBlockSize;
    mNumChannels  = numChannels;
    mPrepared     = true;
    recreate();
}

void Oversampler::setFactor(int factor)
{
    jassert(factor >= 0 && factor <= 5);
    if (factor == mFactor)
        return;
    mFactor = factor;
    if (mPrepared)
        recreate();
}

int Oversampler::getFactor() const
{
    return mFactor;
}

float Oversampler::getLatencySamples() const
{
    if (mOversampling)
        return mOversampling->getLatencyInSamples();
    return 0.0f;
}

juce::dsp::AudioBlock<float> Oversampler::upsample(juce::AudioBuffer<float>& buffer)
{
    if (mFactor == 0 || !mOversampling)
        return juce::dsp::AudioBlock<float>(buffer);

    juce::dsp::AudioBlock<float> inputBlock(buffer);
    return mOversampling->processSamplesUp(inputBlock);
}

void Oversampler::downsample(juce::AudioBuffer<float>& buffer)
{
    if (mFactor == 0 || !mOversampling)
        return;

    juce::dsp::AudioBlock<float> outputBlock(buffer);
    mOversampling->processSamplesDown(outputBlock);
}

void Oversampler::recreate()
{
    if (mFactor == 0)
    {
        mOversampling.reset();
        return;
    }

    mOversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        static_cast<size_t>(mNumChannels),
        static_cast<size_t>(mFactor),
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true
    );

    mOversampling->initProcessing(static_cast<size_t>(mMaxBlockSize));
    mOversampling->reset();
}
