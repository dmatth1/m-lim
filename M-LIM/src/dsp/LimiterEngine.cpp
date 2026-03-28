#include "LimiterEngine.h"
#include "DspUtil.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>
#include <cstring>

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
    mCurrentOversamplingFactor = mOversamplingFactor.load(std::memory_order_relaxed);

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
    mTransientLimiter.prepare(usSampleRate, usBlockSize, numChannels, sampleRate);
    mTransientLimiter.setLookahead(mLookaheadMs.load(std::memory_order_relaxed));
    mTransientLimiter.setChannelLink(mChannelLinkTransients.load(std::memory_order_relaxed) * 0.01f);
    mTransientLimiter.setAlgorithmParams(
        getAlgorithmParams(static_cast<LimiterAlgorithm>(mAlgorithm.load(std::memory_order_relaxed))));

    mLevelingLimiter.prepare(usSampleRate, usBlockSize, numChannels);
    mLevelingLimiter.setAttack(mAttackMs.load(std::memory_order_relaxed));
    mLevelingLimiter.setRelease(mReleaseMs.load(std::memory_order_relaxed));
    mLevelingLimiter.setChannelLink(mChannelLinkRelease.load(std::memory_order_relaxed) * 0.01f);
    mLevelingLimiter.setAlgorithmParams(
        getAlgorithmParams(static_cast<LimiterAlgorithm>(mAlgorithm.load(std::memory_order_relaxed))));

    // Pass the output ceiling to both limiters so they limit smoothly to
    // the correct level rather than relying on the hard-clip at step 7.
    {
        const float inputGain = mInputGainLinear.load(std::memory_order_relaxed);
        const float ceiling = mUnityGain.load(std::memory_order_relaxed)
            ? (1.0f / std::max(inputGain, kDspUtilMinGain))
            : mOutputCeilingLinear.load(std::memory_order_relaxed);
        mTransientLimiter.setThreshold(ceiling);
        mLevelingLimiter.setThreshold(ceiling);
    }

    // Sidechain filter at the original rate (before oversampling)
    mSidechainFilter.prepare(sampleRate, maxBlockSize);

    // True peak detectors at the original output rate
    for (auto& tp : mTruePeakDetectors)
        tp.prepare(sampleRate);
    for (auto& tp : mTruePeakEnforcers)
        tp.prepare(sampleRate);

    // DC filters at original rate
    for (auto& dcf : mDCFilters)
        dcf.prepare(sampleRate);

    // Dithers at original rate
    {
        const int bitDepth    = mDitherBitDepth.load(std::memory_order_relaxed);
        const int noiseShaping = mDitherNoiseShaping.load(std::memory_order_relaxed);
        for (auto& d : mDithers)
        {
            d.prepare(sampleRate);
            d.setBitDepth(bitDepth);
            d.setNoiseShaping(noiseShaping);
        }
    }

    // Pre-allocate working buffers so process() has no heap allocations
    mPreLimitBuffer.setSize(numChannels, maxBlockSize);
    mSidechainBuffer.setSize(numChannels, maxBlockSize);
    mUpPtrs.resize(numChannels);
    mSidePtrs.resize(numChannels);

    mParamsDirty.store(false, std::memory_order_relaxed);
    mDeferredOversamplingChange.store(false, std::memory_order_relaxed);
}

// ============================================================================
// reset — clear all DSP state without reallocating
// ============================================================================
void LimiterEngine::reset()
{
    // TransientLimiter: clear delay buffers, gain state, sliding-window deques
    mTransientLimiter.resetCounters();

    // LevelingLimiter: clear gain/envelope state without reallocating vectors
    mLevelingLimiter.reset();

    // True peak detectors
    for (auto& tp : mTruePeakDetectors)
        tp.reset();
    for (auto& tp : mTruePeakEnforcers)
        tp.reset();

    // DC filters
    for (auto& dcf : mDCFilters)
        dcf.reset();

    // Dither state — clear noise-shaping error buffers without recomputing coefficients
    for (auto& d : mDithers)
        d.reset();

    // Sidechain filter — clear IIR state without reallocating coefficients
    mSidechainFilter.reset();

    // Oversampler filter state — clear internal IIR state without reallocating
    mOversampler.reset();
    mSidechainOversampler.reset();

    // Clear working buffers
    mPreLimitBuffer.clear();
    mSidechainBuffer.clear();

    // Reset metering state
    mGRdB.store (0.0f);
    mTruePkL.store (0.0f);
    mTruePkR.store (0.0f);

    // Clear stale meter snapshots so the UI sees silence after reset
    MeterData discard;
    while (mMeterFIFO.pop(discard)) {}
}

// ============================================================================
// applyPendingParams — called at start of each process() if dirty
// ============================================================================
void LimiterEngine::applyPendingParams()
{
    if (!mParamsDirty.exchange(false, std::memory_order_relaxed))
        return;

    const int newFactor = mOversamplingFactor.load(std::memory_order_relaxed);
    if (newFactor != mCurrentOversamplingFactor)
    {
        // Oversampling factor changed — cannot allocate on the audio thread.
        // Set the deferred flag; the PluginProcessor's AsyncUpdater will schedule
        // a full prepare() on the message thread.  Until then, keep processing
        // with the current (old) oversampling factor.
        mDeferredOversamplingChange.store(true, std::memory_order_relaxed);
        // mParamsDirty was already cleared by the exchange(false) above.
        return;
    }

    // Update parameters that don't require full reprepare
    const LimiterAlgorithm algo = static_cast<LimiterAlgorithm>(mAlgorithm.load(std::memory_order_relaxed));
    AlgorithmParams params = getAlgorithmParams(algo);

    mTransientLimiter.setLookahead(mLookaheadMs.load(std::memory_order_relaxed));
    mTransientLimiter.setChannelLink(mChannelLinkTransients.load(std::memory_order_relaxed) * 0.01f);
    mTransientLimiter.setAlgorithmParams(params);

    mLevelingLimiter.setAttack(mAttackMs.load(std::memory_order_relaxed));
    mLevelingLimiter.setRelease(mReleaseMs.load(std::memory_order_relaxed));
    mLevelingLimiter.setChannelLink(mChannelLinkRelease.load(std::memory_order_relaxed) * 0.01f);
    mLevelingLimiter.setAlgorithmParams(params);

    // Keep limiter thresholds in sync with the current output ceiling
    {
        const float inputGain = mInputGainLinear.load(std::memory_order_relaxed);
        const float ceiling = mUnityGain.load(std::memory_order_relaxed)
            ? (1.0f / std::max(inputGain, kDspUtilMinGain))
            : mOutputCeilingLinear.load(std::memory_order_relaxed);
        mTransientLimiter.setThreshold(ceiling);
        mLevelingLimiter.setThreshold(ceiling);
    }

    {
        const int bitDepth     = mDitherBitDepth.load(std::memory_order_relaxed);
        const int noiseShaping = mDitherNoiseShaping.load(std::memory_order_relaxed);
        for (auto& d : mDithers)
        {
            d.setBitDepth(bitDepth);
            d.setNoiseShaping(noiseShaping);
        }
    }
}

// ============================================================================
// peakLevel — peak-scan helper (no instance state)
// ============================================================================
float LimiterEngine::peakLevel(const juce::AudioBuffer<float>& buf,
                                int channel, int numSamples) noexcept
{
    const float* ptr = buf.getReadPointer(channel);
    float level = 0.0f;
    for (int s = 0; s < numSamples; ++s)
        level = std::max(level, std::abs(ptr[s]));
    return level;
}

// ============================================================================
// process — main DSP chain (orchestrates numbered step methods below)
// ============================================================================
void LimiterEngine::process(juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    applyPendingParams();
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = std::min(buffer.getNumChannels(), mNumChannels);
    if (numSamples == 0 || numChannels == 0)
        return;

    // Guard: if prepare() has not been called, working buffers are empty.
    // Silently pass through to avoid crashing during host plugin scanning or
    // race conditions where processBlock runs before prepareToPlay.
    if (mUpPtrs.empty() || mSidePtrs.empty())
        return;

    // Guard: if the host sends a block larger than we prepared for, clamp to
    // avoid out-of-bounds writes into pre-allocated working buffers.
    if (numSamples > mMaxBlockSize)
        return;
    const float inLevelL = peakLevel(buffer, 0, numSamples);
    const float inLevelR = (numChannels > 1) ? peakLevel(buffer, 1, numSamples) : inLevelL;
    if (mBypass.load(std::memory_order_relaxed))
    {
        // Transparent bypass: pass audio through the full delay path (upsample →
        // TransientLimiter delay buffer at unity gain → downsample) so the output
        // latency matches getLatencySamples() and host delay compensation stays valid.
        juce::dsp::AudioBlock<float> upBlock, upSideBlock;
        stepUpsample(buffer, upBlock, upSideBlock);
        const int upSamples  = static_cast<int>(upBlock.getNumSamples());
        const int upChannels = static_cast<int>(upBlock.getNumChannels());
        mTransientLimiter.processBypassDelay(mUpPtrs.data(), upChannels, upSamples);
        mOversampler.downsample(buffer);
        pushBypassMeterData(buffer, inLevelL, inLevelR, numChannels, numSamples);
        return;
    }
    const float inputGain = stepApplyInputGain(buffer, numChannels, numSamples);
    stepRunSidechainFilter(buffer, numChannels, numSamples);
    juce::dsp::AudioBlock<float> upBlock, upSideBlock;
    stepUpsample(buffer, upBlock, upSideBlock);
    stepRunLimiters(upBlock);
    mOversampler.downsample(buffer);
    const float ceiling = stepApplyCeiling(buffer, numChannels, numSamples, inputGain);
    stepDCFilter(buffer, numChannels, numSamples);
    stepDither(buffer, numChannels, numSamples);
    stepEnforceTruePeak(buffer, numChannels, numSamples, ceiling);
    stepDeltaMode(buffer, numChannels, numSamples);
    const float combinedMinGain = mTransientLimiter.getMinGainLinear() * mLevelingLimiter.getMinGainLinear();
    const float totalGR = juce::jmax(gainToDecibels(std::max(combinedMinGain, kDspUtilMinGain)), -60.0f);
    snapAndPushMeterData(buffer, inLevelL, inLevelR, totalGR, numChannels, numSamples);
}

// ============================================================================
// process() step methods
// ============================================================================

void LimiterEngine::pushBypassMeterData(const juce::AudioBuffer<float>& buffer,
                                         float inLevelL, float inLevelR,
                                         int numChannels, int numSamples)
{
    mGRdB.store(0.0f, std::memory_order_relaxed);
    const float outL = peakLevel(buffer, 0, numSamples);
    const float outR = (numChannels > 1) ? peakLevel(buffer, 1, numSamples) : outL;
    mTruePkL.store(outL, std::memory_order_relaxed);
    mTruePkR.store(outR, std::memory_order_relaxed);
    MeterData md;
    md.inputLevelL    = inLevelL;
    md.inputLevelR    = inLevelR;
    md.outputLevelL   = outL;
    md.outputLevelR   = outR;
    md.gainReduction  = 0.0f;
    md.truePeakL      = outL;
    md.truePeakR      = outR;
    md.waveformSample = 0.0f;
    mMeterFIFO.push(md);
}

float LimiterEngine::stepApplyInputGain(juce::AudioBuffer<float>& buffer,
                                         int numChannels, int numSamples)
{
    const float inputGain = mInputGainLinear.load(std::memory_order_relaxed);
    buffer.applyGain(inputGain);
    // Snapshot pre-limit audio for delta mode (pre-allocated — no heap alloc)
    if (mDeltaMode.load(std::memory_order_relaxed))
    {
        for (int ch = 0; ch < numChannels; ++ch)
            mPreLimitBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }
    return inputGain;
}

void LimiterEngine::stepRunSidechainFilter(juce::AudioBuffer<float>& buffer,
                                            int numChannels, int numSamples)
{
    // Copy main audio into pre-allocated sidechain buffer, then filter
    for (int ch = 0; ch < numChannels; ++ch)
        mSidechainBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    mSidechainFilter.process(mSidechainBuffer);
}

void LimiterEngine::stepUpsample(juce::AudioBuffer<float>& buffer,
                                  juce::dsp::AudioBlock<float>& upBlock,
                                  juce::dsp::AudioBlock<float>& upSideBlock)
{
    // Upsample both main and sidechain so limiters receive equal-length buffers.
    // Upsampling sidechain separately via mSidechainOversampler avoids OOB reads.
    upBlock     = mOversampler.upsample(buffer);
    upSideBlock = mSidechainOversampler.upsample(mSidechainBuffer);
    const int upChannels = static_cast<int>(upBlock.getNumChannels());
    for (int ch = 0; ch < upChannels; ++ch)
    {
        mUpPtrs[ch]   = upBlock.getChannelPointer(ch);
        mSidePtrs[ch] = upSideBlock.getChannelPointer(ch);
    }
}

void LimiterEngine::stepRunLimiters(const juce::dsp::AudioBlock<float>& upBlock)
{
    const int upSamples  = static_cast<int>(upBlock.getNumSamples());
    const int upChannels = static_cast<int>(upBlock.getNumChannels());
    mTransientLimiter.process(mUpPtrs.data(), upChannels, upSamples, mSidePtrs.data());
    // Stage 2 always detects on post-Stage-1 main audio.
    mLevelingLimiter.process(mUpPtrs.data(), upChannels, upSamples);
}

float LimiterEngine::stepApplyCeiling(juce::AudioBuffer<float>& buffer,
                                       int numChannels, int numSamples, float inputGain)
{
    const float ceiling = mUnityGain.load(std::memory_order_relaxed)
        ? (1.0f / std::max(inputGain, kDspUtilMinGain))
        : mOutputCeilingLinear.load(std::memory_order_relaxed);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        for (int s = 0; s < numSamples; ++s)
            data[s] = std::max(-ceiling, std::min(ceiling, data[s]));
    }
    return ceiling;
}

void LimiterEngine::stepEnforceTruePeak(juce::AudioBuffer<float>& buffer,
                                         int numChannels, int numSamples, float ceiling)
{
    if (!mTruePeakEnabled.load(std::memory_order_relaxed))
        return;

    float worstPeak = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        mTruePeakEnforcers[ch].resetPeak();
        mTruePeakEnforcers[ch].processBlock(buffer.getReadPointer(ch), numSamples);
        worstPeak = std::max(worstPeak, mTruePeakEnforcers[ch].getPeak());
    }

    if (worstPeak > ceiling)
    {
        const float gain = ceiling / worstPeak;
        buffer.applyGain(0, numSamples, gain);

        // Reprocess the corrected buffer so FIR state reflects the actual output.
        // Using resetPeak() + processBlock() instead of reset() preserves FIR
        // continuity across blocks, avoiding a cold-FIR warm-up gap (~12 samples).
        for (int ch = 0; ch < numChannels; ++ch)
        {
            mTruePeakEnforcers[ch].resetPeak();
            mTruePeakEnforcers[ch].processBlock(buffer.getReadPointer(ch), numSamples);
        }
    }
}

void LimiterEngine::stepDCFilter(juce::AudioBuffer<float>& buffer,
                                  int numChannels, int numSamples)
{
    if (!mDCFilterEnabled.load(std::memory_order_relaxed))
        return;
    for (int ch = 0; ch < numChannels; ++ch)
        mDCFilters[ch].process(buffer.getWritePointer(ch), numSamples);
}

void LimiterEngine::stepDither(juce::AudioBuffer<float>& buffer,
                                int numChannels, int numSamples)
{
    if (!mDitherEnabled.load(std::memory_order_relaxed))
        return;
    for (int ch = 0; ch < numChannels; ++ch)
        mDithers[ch].process(buffer.getWritePointer(ch), numSamples);
}

void LimiterEngine::stepDeltaMode(juce::AudioBuffer<float>& buffer,
                                   int numChannels, int numSamples)
{
    if (!mDeltaMode.load(std::memory_order_relaxed))
        return;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* pre  = mPreLimitBuffer.getReadPointer(ch);
        float*       post = buffer.getWritePointer(ch);
        for (int s = 0; s < numSamples; ++s)
            post[s] = pre[s] - post[s];
    }
}

void LimiterEngine::snapAndPushMeterData(const juce::AudioBuffer<float>& buffer,
                                         float inLevelL, float inLevelR,
                                         float totalGR, int numChannels, int numSamples)
{
    float outLevelL = peakLevel(buffer, 0, numSamples);
    float outLevelR = (numChannels > 1) ? peakLevel(buffer, 1, numSamples) : outLevelL;

    // True peak detection
    if (mTruePeakEnabled.load(std::memory_order_relaxed))
    {
        for (int ch = 0; ch < kMaxChannels; ++ch)
        {
            const int srcCh = (ch < numChannels) ? ch : 0;
            mTruePeakDetectors[ch].processBlock(buffer.getReadPointer(srcCh), numSamples);
        }
    }

    mGRdB.store(totalGR, std::memory_order_relaxed);
    mTruePkL.store(mTruePeakDetectors[0].getPeak(), std::memory_order_relaxed);
    mTruePkR.store(mTruePeakDetectors[1].getPeak(), std::memory_order_relaxed);

    // Push meter data to FIFO (non-blocking — drop if full)
    MeterData md;
    md.inputLevelL   = inLevelL;
    md.inputLevelR   = inLevelR;
    md.outputLevelL  = outLevelL;
    md.outputLevelR  = outLevelR;
    md.gainReduction = totalGR;
    md.truePeakL     = mTruePeakDetectors[0].getPeak();
    md.truePeakR     = mTruePeakDetectors[1].getPeak();

    // Waveform: one GR sample per block for the scrolling WaveformDisplay
    md.waveformSample = totalGR;
    mMeterFIFO.push(md);
}

// ============================================================================
// Parameter setters
// ============================================================================


void LimiterEngine::setAlgorithm(LimiterAlgorithm algo)
{
    const int v = static_cast<int>(algo);
    if (mAlgorithm.load(std::memory_order_relaxed) == v)
        return;
    mAlgorithm.store(v, std::memory_order_relaxed);
    mParamsDirty.store(true, std::memory_order_relaxed);
}

void LimiterEngine::setInputGain(float dB)
{
    setIfChanged(mInputGainLinear, decibelsToGain(dB));
}

void LimiterEngine::setOutputCeiling(float dB)
{
    setIfChanged(mOutputCeilingLinear, decibelsToGain(dB));
}

void LimiterEngine::setLookahead(float ms)           { setIfChanged(mLookaheadMs, ms); }
void LimiterEngine::setAttack(float ms)               { setIfChanged(mAttackMs, ms); }
void LimiterEngine::setRelease(float ms)              { setIfChanged(mReleaseMs, ms); }
void LimiterEngine::setChannelLinkTransients(float pct) { setIfChanged(mChannelLinkTransients, pct); }
void LimiterEngine::setChannelLinkRelease(float pct)  { setIfChanged(mChannelLinkRelease, pct); }

void LimiterEngine::setOversamplingFactor(int factor)
{
    // No change guard here — setOversamplingFactor has its own deferred change
    // detection path via mCurrentOversamplingFactor in applyPendingParams().
    mOversamplingFactor.store(std::max(Oversampler::kMinOversamplingFactor,
                                       std::min(Oversampler::kMaxOversamplingFactor, factor)),
                              std::memory_order_relaxed);
    mParamsDirty.store(true, std::memory_order_relaxed);
}

void LimiterEngine::setTruePeakEnabled(bool enabled)
{
    mTruePeakEnabled.store(enabled, std::memory_order_relaxed);
}

void LimiterEngine::setDCFilterEnabled(bool enabled)
{
    mDCFilterEnabled.store(enabled, std::memory_order_relaxed);
}

void LimiterEngine::setDitherEnabled(bool enabled)
{
    mDitherEnabled.store(enabled, std::memory_order_relaxed);
}

void LimiterEngine::setDitherBitDepth(int bits)     { setIfChanged(mDitherBitDepth, bits); }
void LimiterEngine::setDitherNoiseShaping(int mode) { setIfChanged(mDitherNoiseShaping, mode); }

void LimiterEngine::setBypass(bool bypass)
{
    mBypass.store(bypass, std::memory_order_relaxed);
}

void LimiterEngine::setDeltaMode(bool delta)
{
    mDeltaMode.store(delta, std::memory_order_relaxed);
}

void LimiterEngine::setUnityGain(bool unity)  { setIfChanged(mUnityGain, unity); }

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
    return mGRdB.load(std::memory_order_relaxed);
}

float LimiterEngine::getTruePeakL() const
{
    return mTruePkL.load(std::memory_order_relaxed);
}

float LimiterEngine::getTruePeakR() const
{
    return mTruePkR.load(std::memory_order_relaxed);
}

int LimiterEngine::getLatencySamples() const
{
    const int lookaheadSamples   = mTransientLimiter.getLatencyInSamples();
    const int oversamplerLatency = static_cast<int>(std::lround(mOversampler.getLatencySamples()));
    return lookaheadSamples + oversamplerLatency;
}

float LimiterEngine::getOversamplerLatency() const
{
    return mOversampler.getLatencySamples();
}

float LimiterEngine::getLookaheadMs() const
{
    return mLookaheadMs.load(std::memory_order_relaxed);
}
