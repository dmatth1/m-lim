#include "LimiterEngine.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

static constexpr float kMinLinear = 1e-6f;  // -120 dB floor

// ============================================================================
// Constructor
// ============================================================================
LimiterEngine::LimiterEngine()
{
    // Default state — components will be properly prepared in prepare()
}

// ============================================================================
// prepare
// ============================================================================
void LimiterEngine::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    mSampleRate          = sampleRate;
    mMaxBlockSize        = maxBlockSize;
    mNumChannels         = numChannels;
    mCurrentOversamplingFactor = mOversamplingFactor.load();

    // Prepare oversampler first (determines upsampled rate)
    mOversampler.setFactor(mCurrentOversamplingFactor);
    mOversampler.prepare(sampleRate, maxBlockSize, numChannels);

    const int   osFactorPow2 = (mCurrentOversamplingFactor > 0)
                                   ? (1 << mCurrentOversamplingFactor) : 1;
    const double usSampleRate = sampleRate * osFactorPow2;
    const int    usBlockSize  = maxBlockSize * osFactorPow2;

    // Prepare limiters at the upsampled rate
    mTransientLimiter.prepare(usSampleRate, usBlockSize, numChannels);
    mTransientLimiter.setLookahead(mLookaheadMs.load());
    mTransientLimiter.setChannelLink(mChannelLinkTransients.load());
    mTransientLimiter.setAlgorithmParams(
        getAlgorithmParams(static_cast<LimiterAlgorithm>(mAlgorithm.load())));

    mLevelingLimiter.prepare(usSampleRate, usBlockSize, numChannels);
    mLevelingLimiter.setAttack(mAttackMs.load());
    mLevelingLimiter.setRelease(mReleaseMs.load());
    mLevelingLimiter.setChannelLink(mChannelLinkRelease.load());
    mLevelingLimiter.setAlgorithmParams(
        getAlgorithmParams(static_cast<LimiterAlgorithm>(mAlgorithm.load())));

    // Sidechain filter at the original rate (before oversampling)
    mSidechainFilter.prepare(sampleRate, maxBlockSize);

    // True peak detectors at the original output rate
    mTruePeakL.prepare(sampleRate);
    mTruePeakR.prepare(sampleRate);

    // DC filters at original rate
    mDCFilterL.prepare(sampleRate);
    mDCFilterR.prepare(sampleRate);

    // Dithers at original rate
    mDitherL.prepare(sampleRate);
    mDitherR.prepare(sampleRate);
    mDitherL.setBitDepth(mDitherBitDepth.load());
    mDitherR.setBitDepth(mDitherBitDepth.load());
    mDitherL.setNoiseShaping(mDitherNoiseShaping.load());
    mDitherR.setNoiseShaping(mDitherNoiseShaping.load());

    // Allocate sidechain working buffers
    mSidechainData.assign(numChannels, std::vector<float>(maxBlockSize, 0.0f));

    mParamsDirty.store(false);
}

// ============================================================================
// applyPendingParams — called at start of each process() if dirty
// ============================================================================
void LimiterEngine::applyPendingParams()
{
    if (!mParamsDirty.exchange(false))
        return;

    const int newFactor = mOversamplingFactor.load();
    if (newFactor != mCurrentOversamplingFactor)
    {
        // Oversampling factor changed — full reprepare needed
        mCurrentOversamplingFactor = newFactor;
        prepare(mSampleRate, mMaxBlockSize, mNumChannels);
        return;  // prepare() clears dirty flag
    }

    // Update parameters that don't require full reprepare
    const LimiterAlgorithm algo = static_cast<LimiterAlgorithm>(mAlgorithm.load());
    AlgorithmParams params = getAlgorithmParams(algo);

    mTransientLimiter.setLookahead(mLookaheadMs.load());
    mTransientLimiter.setChannelLink(mChannelLinkTransients.load());
    mTransientLimiter.setAlgorithmParams(params);

    mLevelingLimiter.setAttack(mAttackMs.load());
    mLevelingLimiter.setRelease(mReleaseMs.load());
    mLevelingLimiter.setChannelLink(mChannelLinkRelease.load());
    mLevelingLimiter.setAlgorithmParams(params);

    mDitherL.setBitDepth(mDitherBitDepth.load());
    mDitherR.setBitDepth(mDitherBitDepth.load());
    mDitherL.setNoiseShaping(mDitherNoiseShaping.load());
    mDitherR.setNoiseShaping(mDitherNoiseShaping.load());
}

// ============================================================================
// process — main DSP chain
// ============================================================================
void LimiterEngine::process(juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    applyPendingParams();

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = std::min(buffer.getNumChannels(), mNumChannels);

    if (numSamples == 0 || numChannels == 0)
        return;

    // ------------------------------------------------------------------
    // Snapshot input levels for metering
    // ------------------------------------------------------------------
    float inLevelL = 0.0f, inLevelR = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        inLevelL = std::max(inLevelL, std::abs(buffer.getSample(0, i)));
        if (numChannels > 1)
            inLevelR = std::max(inLevelR, std::abs(buffer.getSample(1, i)));
        else
            inLevelR = inLevelL;
    }

    // ------------------------------------------------------------------
    // Bypass mode: pass through unchanged, still update meters
    // ------------------------------------------------------------------
    if (mBypass.load())
    {
        mGRdB.store(0.0f);
        float outL = 0.0f, outR = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            outL = std::max(outL, std::abs(buffer.getSample(0, i)));
            if (numChannels > 1)
                outR = std::max(outR, std::abs(buffer.getSample(1, i)));
        }
        if (numChannels <= 1) outR = outL;
        mTruePkL.store(outL);
        mTruePkR.store(outR);
        return;
    }

    // ------------------------------------------------------------------
    // Step 1: Apply input gain
    // ------------------------------------------------------------------
    const float inputGain = mInputGainLinear.load();
    buffer.applyGain(inputGain);

    // Optionally store pre-limit buffer for delta mode
    juce::AudioBuffer<float> preLimitBuffer;
    if (mDeltaMode.load())
    {
        preLimitBuffer.makeCopyOf(buffer);
    }

    // ------------------------------------------------------------------
    // Step 2: Build sidechain copy and run sidechain filter on it
    // ------------------------------------------------------------------
    juce::AudioBuffer<float> sideBuf(buffer);
    mSidechainFilter.process(sideBuf);

    // Copy to sidechain float** arrays
    std::vector<const float*> sidePtrs(numChannels);
    for (int ch = 0; ch < numChannels; ++ch)
        sidePtrs[ch] = sideBuf.getReadPointer(ch);

    // ------------------------------------------------------------------
    // Step 3: Oversample up
    // ------------------------------------------------------------------
    juce::dsp::AudioBlock<float> upBlock = mOversampler.upsample(buffer);
    const int upSamples  = static_cast<int>(upBlock.getNumSamples());
    const int upChannels = static_cast<int>(upBlock.getNumChannels());

    std::vector<float*> upPtrs(upChannels);
    for (int ch = 0; ch < upChannels; ++ch)
        upPtrs[ch] = upBlock.getChannelPointer(ch);

    // ------------------------------------------------------------------
    // Step 4: TransientLimiter (Stage 1)
    // ------------------------------------------------------------------
    mTransientLimiter.process(upPtrs.data(), upChannels, upSamples,
                               sidePtrs.data());

    // ------------------------------------------------------------------
    // Step 5: LevelingLimiter (Stage 2)
    // ------------------------------------------------------------------
    mLevelingLimiter.process(upPtrs.data(), upChannels, upSamples,
                              sidePtrs.data());

    // ------------------------------------------------------------------
    // Step 6: Oversample down
    // ------------------------------------------------------------------
    mOversampler.downsample(buffer);

    // ------------------------------------------------------------------
    // Step 7: Apply output ceiling (hard clip)
    // ------------------------------------------------------------------
    const float ceiling = mUnityGain.load()
        ? (1.0f / std::max(inputGain, kMinLinear))
        : mOutputCeilingLinear.load();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            data[i] = std::max(-ceiling, std::min(ceiling, data[i]));
    }

    // ------------------------------------------------------------------
    // Step 8: DC filter (optional)
    // ------------------------------------------------------------------
    if (mDCFilterEnabled.load())
    {
        mDCFilterL.process(buffer.getWritePointer(0), numSamples);
        if (numChannels > 1)
            mDCFilterR.process(buffer.getWritePointer(1), numSamples);
    }

    // ------------------------------------------------------------------
    // Step 9: Dither (optional)
    // ------------------------------------------------------------------
    if (mDitherEnabled.load())
    {
        mDitherL.process(buffer.getWritePointer(0), numSamples);
        if (numChannels > 1)
            mDitherR.process(buffer.getWritePointer(1), numSamples);
    }

    // ------------------------------------------------------------------
    // Step 10: Delta mode — output what the limiter removed
    // ------------------------------------------------------------------
    if (mDeltaMode.load() && preLimitBuffer.getNumSamples() == numSamples)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* pre  = preLimitBuffer.getReadPointer(ch);
            float*       post = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
                post[i] = pre[i] - post[i];
        }
    }

    // ------------------------------------------------------------------
    // Step 11: Meter — measure output and true peak
    // ------------------------------------------------------------------
    float outLevelL = 0.0f, outLevelR = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        outLevelL = std::max(outLevelL, std::abs(buffer.getSample(0, i)));
        if (numChannels > 1)
            outLevelR = std::max(outLevelR, std::abs(buffer.getSample(1, i)));
    }
    if (numChannels <= 1) outLevelR = outLevelL;

    // True peak detection
    if (mTruePeakEnabled.load())
    {
        mTruePeakL.processBlock(buffer.getReadPointer(0), numSamples);
        if (numChannels > 1)
            mTruePeakR.processBlock(buffer.getReadPointer(1), numSamples);
        else
            mTruePeakR.processBlock(buffer.getReadPointer(0), numSamples);
    }

    // Combined GR from both stages
    const float grL = mTransientLimiter.getGainReduction();
    const float grS = mLevelingLimiter.getGainReduction();
    const float totalGR = juce::jmax(grL + grS, -60.0f);  // sum both stages (both ≤ 0 dB), clamp floor
    mGRdB.store(totalGR);
    mTruePkL.store(mTruePeakL.getPeak());
    mTruePkR.store(mTruePeakR.getPeak());

    // Push meter data to FIFO (non-blocking — drop if full)
    MeterData md;
    md.inputLevelL   = inLevelL;
    md.inputLevelR   = inLevelR;
    md.outputLevelL  = outLevelL;
    md.outputLevelR  = outLevelR;
    md.gainReduction = totalGR;
    md.truePeakL     = mTruePeakL.getPeak();
    md.truePeakR     = mTruePeakR.getPeak();

    // Waveform: copy output level snapshot (one value per block for scrolling display)
    md.waveformSize       = 1;
    md.waveformBuffer[0]  = totalGR;  // GR trace for waveform display
    mMeterFIFO.push(md);
}

// ============================================================================
// Parameter setters
// ============================================================================
void LimiterEngine::setAlgorithm(LimiterAlgorithm algo)
{
    mAlgorithm.store(static_cast<int>(algo));
    mParamsDirty.store(true);
}

void LimiterEngine::setInputGain(float dB)
{
    mInputGainLinear.store(std::pow(10.0f, dB / 20.0f));
}

void LimiterEngine::setOutputCeiling(float dB)
{
    mOutputCeilingLinear.store(std::pow(10.0f, dB / 20.0f));
}

void LimiterEngine::setLookahead(float ms)
{
    mLookaheadMs.store(ms);
    mParamsDirty.store(true);
}

void LimiterEngine::setAttack(float ms)
{
    mAttackMs.store(ms);
    mParamsDirty.store(true);
}

void LimiterEngine::setRelease(float ms)
{
    mReleaseMs.store(ms);
    mParamsDirty.store(true);
}

void LimiterEngine::setChannelLinkTransients(float pct)
{
    mChannelLinkTransients.store(pct);
    mParamsDirty.store(true);
}

void LimiterEngine::setChannelLinkRelease(float pct)
{
    mChannelLinkRelease.store(pct);
    mParamsDirty.store(true);
}

void LimiterEngine::setOversamplingFactor(int factor)
{
    mOversamplingFactor.store(std::max(0, std::min(5, factor)));
    mParamsDirty.store(true);
}

void LimiterEngine::setTruePeakEnabled(bool enabled)
{
    mTruePeakEnabled.store(enabled);
}

void LimiterEngine::setDCFilterEnabled(bool enabled)
{
    mDCFilterEnabled.store(enabled);
}

void LimiterEngine::setDitherEnabled(bool enabled)
{
    mDitherEnabled.store(enabled);
}

void LimiterEngine::setDitherBitDepth(int bits)
{
    mDitherBitDepth.store(bits);
    mParamsDirty.store(true);
}

void LimiterEngine::setDitherNoiseShaping(int mode)
{
    mDitherNoiseShaping.store(mode);
    mParamsDirty.store(true);
}

void LimiterEngine::setBypass(bool bypass)
{
    mBypass.store(bypass);
}

void LimiterEngine::setDeltaMode(bool delta)
{
    mDeltaMode.store(delta);
}

void LimiterEngine::setUnityGain(bool unity)
{
    mUnityGain.store(unity);
}

// ============================================================================
// State queries
// ============================================================================
float LimiterEngine::getGainReduction() const
{
    return mGRdB.load();
}

float LimiterEngine::getTruePeakL() const
{
    return mTruePkL.load();
}

float LimiterEngine::getTruePeakR() const
{
    return mTruePkR.load();
}

int LimiterEngine::getLatencySamples() const
{
    // Lookahead latency (at original sample rate) + oversampler latency
    const float lookaheadMs = mLookaheadMs.load();
    const int lookaheadSamples = static_cast<int>(lookaheadMs * 0.001 * mSampleRate);
    const int oversamplerLatency = static_cast<int>(mOversampler.getLatencySamples());
    return lookaheadSamples + oversamplerLatency;
}
