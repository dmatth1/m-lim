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

    // Sidechain oversampler mirrors the main oversampler so sidechain data is
    // the same length as the upsampled main audio when passed to the limiters.
    mSidechainOversampler.setFactor(mCurrentOversamplingFactor);
    mSidechainOversampler.prepare(sampleRate, maxBlockSize, numChannels);

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

    // Pass the output ceiling to both limiters so they limit smoothly to
    // the correct level rather than relying on the hard-clip at step 7.
    {
        const float inputGain = mInputGainLinear.load();
        const float ceiling = mUnityGain.load()
            ? (1.0f / std::max(inputGain, kMinLinear))
            : mOutputCeilingLinear.load();
        mTransientLimiter.setThreshold(ceiling);
        mLevelingLimiter.setThreshold(ceiling);
    }

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

    // Pre-allocate working buffers so process() has no heap allocations
    mPreLimitBuffer.setSize(numChannels, maxBlockSize);
    mSidechainBuffer.setSize(numChannels, maxBlockSize);
    mUpPtrs.resize(numChannels);
    mSidePtrs.resize(numChannels);

    mParamsDirty.store(false);
    mDeferredOversamplingChange.store(false);
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
        // Oversampling factor changed — cannot allocate on the audio thread.
        // Set the deferred flag; the PluginProcessor's AsyncUpdater will schedule
        // a full prepare() on the message thread.  Until then, keep processing
        // with the current (old) oversampling factor.
        mDeferredOversamplingChange.store(true);
        mParamsDirty.store(false);  // suppress repeated triggers this block
        return;
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

    // Keep limiter thresholds in sync with the current output ceiling
    {
        const float inputGain = mInputGainLinear.load();
        const float ceiling = mUnityGain.load()
            ? (1.0f / std::max(inputGain, kMinLinear))
            : mOutputCeilingLinear.load();
        mTransientLimiter.setThreshold(ceiling);
        mLevelingLimiter.setThreshold(ceiling);
    }

    mDitherL.setBitDepth(mDitherBitDepth.load());
    mDitherR.setBitDepth(mDitherBitDepth.load());
    mDitherL.setNoiseShaping(mDitherNoiseShaping.load());
    mDitherR.setNoiseShaping(mDitherNoiseShaping.load());
}

// ============================================================================
// peakLevel — peak-scan helper (no instance state)
// ============================================================================
float LimiterEngine::peakLevel(const juce::AudioBuffer<float>& buf,
                                int channel, int numSamples) noexcept
{
    float level = 0.0f;
    for (int s = 0; s < numSamples; ++s)
        level = std::max(level, std::abs(buf.getSample(channel, s)));
    return level;
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
    float inLevelL = peakLevel(buffer, 0, numSamples);
    float inLevelR = (numChannels > 1) ? peakLevel(buffer, 1, numSamples) : inLevelL;

    // ------------------------------------------------------------------
    // Bypass mode: pass through unchanged, still update meters
    // ------------------------------------------------------------------
    if (mBypass.load())
    {
        mGRdB.store(0.0f);
        float outL = peakLevel(buffer, 0, numSamples);
        float outR = (numChannels > 1) ? peakLevel(buffer, 1, numSamples) : outL;
        mTruePkL.store(outL);
        mTruePkR.store(outR);
        return;
    }

    // ------------------------------------------------------------------
    // Step 1: Apply input gain
    // ------------------------------------------------------------------
    const float inputGain = mInputGainLinear.load();
    buffer.applyGain(inputGain);

    // Optionally store pre-limit buffer for delta mode (pre-allocated — no heap alloc)
    if (mDeltaMode.load())
    {
        for (int ch = 0; ch < numChannels; ++ch)
            mPreLimitBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    // ------------------------------------------------------------------
    // Step 2: Build sidechain copy and run sidechain filter on it
    //         (copy into pre-allocated buffer — no heap alloc)
    // ------------------------------------------------------------------
    for (int ch = 0; ch < numChannels; ++ch)
        mSidechainBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    mSidechainFilter.process(mSidechainBuffer);

    // ------------------------------------------------------------------
    // Step 3: Oversample up — both main audio and sidechain so the buffers
    // are the same length when passed to the oversampled limiters.
    // Upsampling sideBuf separately via mSidechainOversampler avoids the
    // OOB read that would occur if the original (numSamples-long) sidePtrs
    // were passed to limiters expecting upSamples elements.
    // ------------------------------------------------------------------
    juce::dsp::AudioBlock<float> upBlock     = mOversampler.upsample(buffer);
    juce::dsp::AudioBlock<float> upSideBlock = mSidechainOversampler.upsample(mSidechainBuffer);

    const int upSamples  = static_cast<int>(upBlock.getNumSamples());
    const int upChannels = static_cast<int>(upBlock.getNumChannels());

    // Populate pre-allocated pointer arrays (no heap alloc)
    for (int ch = 0; ch < upChannels; ++ch)
    {
        mUpPtrs[ch]  = upBlock.getChannelPointer(ch);
        mSidePtrs[ch] = upSideBlock.getChannelPointer(ch);
    }

    // ------------------------------------------------------------------
    // Step 4: TransientLimiter (Stage 1)
    // ------------------------------------------------------------------
    mTransientLimiter.process(mUpPtrs.data(), upChannels, upSamples,
                               mSidePtrs.data());

    // ------------------------------------------------------------------
    // Step 5: LevelingLimiter (Stage 2)
    // ------------------------------------------------------------------
    mLevelingLimiter.process(mUpPtrs.data(), upChannels, upSamples,
                              mSidePtrs.data());

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
        for (int s = 0; s < numSamples; ++s)
            data[s] = std::max(-ceiling, std::min(ceiling, data[s]));
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
    if (mDeltaMode.load())
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* pre  = mPreLimitBuffer.getReadPointer(ch);
            float*       post = buffer.getWritePointer(ch);
            for (int s = 0; s < numSamples; ++s)
                post[s] = pre[s] - post[s];
        }
    }

    // ------------------------------------------------------------------
    // Step 11: Meter — measure output and true peak
    // ------------------------------------------------------------------
    float outLevelL = peakLevel(buffer, 0, numSamples);
    float outLevelR = (numChannels > 1) ? peakLevel(buffer, 1, numSamples) : outLevelL;

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
    mParamsDirty.store(true);  // unity-gain ceiling depends on input gain
}

void LimiterEngine::setOutputCeiling(float dB)
{
    mOutputCeilingLinear.store(std::pow(10.0f, dB / 20.0f));
    mParamsDirty.store(true);  // limiter thresholds must track ceiling
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
    mOversamplingFactor.store(std::max(Oversampler::kMinOversamplingFactor,
                                       std::min(Oversampler::kMaxOversamplingFactor, factor)));
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
    mParamsDirty.store(true);  // ceiling changes when unity-gain mode changes
}

void LimiterEngine::setSidechainHPFreq(float hz)
{
    mSidechainFilter.setHighPassFreq(hz);
}

void LimiterEngine::setSidechainLPFreq(float hz)
{
    mSidechainFilter.setLowPassFreq(hz);
}

void LimiterEngine::setSidechainTilt(float dB)
{
    mSidechainFilter.setTilt(dB);
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
    const int lookaheadSamples = static_cast<int>(std::round(lookaheadMs * 0.001 * mSampleRate));
    const int oversamplerLatency = static_cast<int>(std::lround(mOversampler.getLatencySamples()));
    return lookaheadSamples + oversamplerLatency;
}

float LimiterEngine::getOversamplerLatency() const
{
    return mOversampler.getLatencySamples();
}

float LimiterEngine::getLookaheadMs() const
{
    return mLookaheadMs.load();
}
