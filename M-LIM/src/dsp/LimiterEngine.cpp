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
            ? (1.0f / std::max(inputGain, kDspUtilMinGain))
            : mOutputCeilingLinear.load();
        mTransientLimiter.setThreshold(ceiling);
        mLevelingLimiter.setThreshold(ceiling);
    }

    // Sidechain filter at the original rate (before oversampling)
    mSidechainFilter.prepare(sampleRate, maxBlockSize);

    // True peak detectors at the original output rate
    mTruePeakL.prepare(sampleRate);
    mTruePeakR.prepare(sampleRate);
    mTruePeakEnforceL.prepare(sampleRate);
    mTruePeakEnforceR.prepare(sampleRate);

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
// reset — clear all DSP state without reallocating
// ============================================================================
void LimiterEngine::reset()
{
    // TransientLimiter: clear delay buffers, gain state, sliding-window deques
    mTransientLimiter.resetCounters();

    // LevelingLimiter: re-prepare to clear gain/envelope state
    // (no dedicated reset — prepare() zeros everything without realloc if sizes match)
    {
        const int osFactorPow2 = (mCurrentOversamplingFactor > 0)
                                     ? (1 << mCurrentOversamplingFactor) : 1;
        const double usSampleRate = mSampleRate * osFactorPow2;
        const int    usBlockSize  = mMaxBlockSize * osFactorPow2;
        mLevelingLimiter.prepare (usSampleRate, usBlockSize, mNumChannels);
    }

    // True peak detectors
    mTruePeakL.reset();
    mTruePeakR.reset();
    mTruePeakEnforceL.reset();
    mTruePeakEnforceR.reset();

    // DC filters
    mDCFilterL.reset();
    mDCFilterR.reset();

    // Dither state — re-prepare to clear noise-shaping error buffers
    mDitherL.prepare (mSampleRate);
    mDitherR.prepare (mSampleRate);

    // Sidechain filter — re-prepare to clear filter state
    mSidechainFilter.prepare (mSampleRate, mMaxBlockSize);

    // Oversampler filter state — re-prepare clears internal IIR state
    mOversampler.prepare (mSampleRate, mMaxBlockSize, mNumChannels);
    mSidechainOversampler.prepare (mSampleRate, mMaxBlockSize, mNumChannels);

    // Clear working buffers
    mPreLimitBuffer.clear();
    mSidechainBuffer.clear();

    // Reset metering state
    mGRdB.store (0.0f);
    mTruePkL.store (0.0f);
    mTruePkR.store (0.0f);
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
        // mParamsDirty was already cleared by the exchange(false) above.
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
            ? (1.0f / std::max(inputGain, kDspUtilMinGain))
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
    const float inLevelL = peakLevel(buffer, 0, numSamples);
    const float inLevelR = (numChannels > 1) ? peakLevel(buffer, 1, numSamples) : inLevelL;
    if (mBypass.load())
    {
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
    stepEnforceTruePeak(buffer, numChannels, numSamples, ceiling);
    stepDither(buffer, numChannels, numSamples);
    stepDeltaMode(buffer, numChannels, numSamples);
    const float totalGR = juce::jmax(mTransientLimiter.getGainReduction() + mLevelingLimiter.getGainReduction(), -60.0f);
    snapAndPushMeterData(buffer, inLevelL, inLevelR, totalGR, numChannels, numSamples);
}

// ============================================================================
// process() step methods
// ============================================================================

void LimiterEngine::pushBypassMeterData(const juce::AudioBuffer<float>& buffer,
                                         float inLevelL, float inLevelR,
                                         int numChannels, int numSamples)
{
    mGRdB.store(0.0f);
    const float outL = peakLevel(buffer, 0, numSamples);
    const float outR = (numChannels > 1) ? peakLevel(buffer, 1, numSamples) : outL;
    mTruePkL.store(outL);
    mTruePkR.store(outR);
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
    const float inputGain = mInputGainLinear.load();
    buffer.applyGain(inputGain);
    // Snapshot pre-limit audio for delta mode (pre-allocated — no heap alloc)
    if (mDeltaMode.load())
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
    // Stage 2 detects on the post-Stage-1 main audio (nullptr = use channelData).
    // Passing mSidePtrs here caused over-limiting: Stage 2 would see the
    // pre-Stage-1 peak and apply additional GR even when Stage 1 already handled it.
    mLevelingLimiter.process(mUpPtrs.data(), upChannels, upSamples, nullptr);
}

float LimiterEngine::stepApplyCeiling(juce::AudioBuffer<float>& buffer,
                                       int numChannels, int numSamples, float inputGain)
{
    const float ceiling = mUnityGain.load()
        ? (1.0f / std::max(inputGain, kDspUtilMinGain))
        : mOutputCeilingLinear.load();
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
    if (!mTruePeakEnabled.load())
        return;

    mTruePeakEnforceL.resetPeak();
    mTruePeakEnforceL.processBlock(buffer.getReadPointer(0), numSamples);
    float tpL = mTruePeakEnforceL.getPeak();

    float tpR = tpL;
    if (numChannels > 1)
    {
        mTruePeakEnforceR.resetPeak();
        mTruePeakEnforceR.processBlock(buffer.getReadPointer(1), numSamples);
        tpR = mTruePeakEnforceR.getPeak();
    }

    const float worstPeak = std::max(tpL, tpR);
    if (worstPeak > ceiling)
    {
        const float gain = ceiling / worstPeak;
        buffer.applyGain(0, numSamples, gain);

        // The FIR state now reflects the pre-correction signal, which is out of
        // sync with the scaled output.  Reset fully so the next block starts with
        // a FIR that matches the actual output rather than the uncorrected samples.
        mTruePeakEnforceL.reset();
        if (numChannels > 1)
            mTruePeakEnforceR.reset();
    }
}

void LimiterEngine::stepDCFilter(juce::AudioBuffer<float>& buffer,
                                  int numChannels, int numSamples)
{
    if (!mDCFilterEnabled.load())
        return;
    mDCFilterL.process(buffer.getWritePointer(0), numSamples);
    if (numChannels > 1)
        mDCFilterR.process(buffer.getWritePointer(1), numSamples);
}

void LimiterEngine::stepDither(juce::AudioBuffer<float>& buffer,
                                int numChannels, int numSamples)
{
    if (!mDitherEnabled.load())
        return;
    mDitherL.process(buffer.getWritePointer(0), numSamples);
    if (numChannels > 1)
        mDitherR.process(buffer.getWritePointer(1), numSamples);
}

void LimiterEngine::stepDeltaMode(juce::AudioBuffer<float>& buffer,
                                   int numChannels, int numSamples)
{
    if (!mDeltaMode.load())
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
    if (mTruePeakEnabled.load())
    {
        mTruePeakL.processBlock(buffer.getReadPointer(0), numSamples);
        if (numChannels > 1)
            mTruePeakR.processBlock(buffer.getReadPointer(1), numSamples);
        else
            mTruePeakR.processBlock(buffer.getReadPointer(0), numSamples);
    }

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
    mAlgorithm.store(v);
    mParamsDirty.store(true);
}

void LimiterEngine::setInputGain(float dB)
{
    const float newLinear = std::pow(10.0f, dB / 20.0f);
    if (floatBitsEqual(mInputGainLinear.load(std::memory_order_relaxed), newLinear))
        return;
    mInputGainLinear.store(newLinear);
    mParamsDirty.store(true);  // unity-gain ceiling depends on input gain
}

void LimiterEngine::setOutputCeiling(float dB)
{
    const float newLinear = std::pow(10.0f, dB / 20.0f);
    if (floatBitsEqual(mOutputCeilingLinear.load(std::memory_order_relaxed), newLinear))
        return;
    mOutputCeilingLinear.store(newLinear);
    mParamsDirty.store(true);  // limiter thresholds must track ceiling
}

void LimiterEngine::setLookahead(float ms)
{
    if (floatBitsEqual(mLookaheadMs.load(std::memory_order_relaxed), ms))
        return;
    mLookaheadMs.store(ms);
    mParamsDirty.store(true);
}

void LimiterEngine::setAttack(float ms)
{
    if (floatBitsEqual(mAttackMs.load(std::memory_order_relaxed), ms))
        return;
    mAttackMs.store(ms);
    mParamsDirty.store(true);
}

void LimiterEngine::setRelease(float ms)
{
    if (floatBitsEqual(mReleaseMs.load(std::memory_order_relaxed), ms))
        return;
    mReleaseMs.store(ms);
    mParamsDirty.store(true);
}

void LimiterEngine::setChannelLinkTransients(float pct)
{
    if (floatBitsEqual(mChannelLinkTransients.load(std::memory_order_relaxed), pct))
        return;
    mChannelLinkTransients.store(pct);
    mParamsDirty.store(true);
}

void LimiterEngine::setChannelLinkRelease(float pct)
{
    if (floatBitsEqual(mChannelLinkRelease.load(std::memory_order_relaxed), pct))
        return;
    mChannelLinkRelease.store(pct);
    mParamsDirty.store(true);
}

void LimiterEngine::setOversamplingFactor(int factor)
{
    // No change guard here — setOversamplingFactor has its own deferred change
    // detection path via mCurrentOversamplingFactor in applyPendingParams().
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
    if (mDitherBitDepth.load(std::memory_order_relaxed) == bits)
        return;
    mDitherBitDepth.store(bits);
    mParamsDirty.store(true);
}

void LimiterEngine::setDitherNoiseShaping(int mode)
{
    if (mDitherNoiseShaping.load(std::memory_order_relaxed) == mode)
        return;
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
    if (mUnityGain.load(std::memory_order_relaxed) == unity)
        return;
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
    const int lookaheadSamples = static_cast<int>(lookaheadMs * 0.001 * mSampleRate);
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
