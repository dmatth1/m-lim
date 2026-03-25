#include "LoudnessMeter.h"

#include <cmath>
#include <algorithm>
#include <numeric>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
static constexpr double kPi              = 3.14159265358979323846;
static constexpr double kLufsCorrectiondB = -0.691; // BS.1770-4: 10*log10(Σ G_i * z_i) - 0.691
static constexpr double kAbsGateLUFS     = -70.0;   // absolute gate threshold
static constexpr double kRelGateOffset   = -10.0;   // relative gate = ungated mean - 10 LU
static constexpr double kBS1770PreFilterFreqHz  = 1681.974450955533;  // ITU-R BS.1770-4 pre-filter Fc
static constexpr double kBS1770RLBHighPassFreqHz = 38.13547087602444; // ITU-R BS.1770-4 RLB HP Fc

// ---------------------------------------------------------------------------
// prepare
// ---------------------------------------------------------------------------
void LoudnessMeter::prepare(double sampleRate, int numChannels)
{
    mSampleRate  = sampleRate;
    mNumChannels = numChannels;

    // 100 ms block size — round to nearest sample
    mBlockSize  = static_cast<int>(std::round(sampleRate * 0.1));
    mBlockAccum = 0;
    mBlockPower = 0.0;

    mMomentaryRing.reset();
    mShortTermRing.reset();

    mMomentaryLUFS.store(kNegInf);
    mShortTermLUFS.store(kNegInf);
    mIntegratedLUFS.store(kNegInf);
    mLoudnessRange.store(0.0f);

    // Pre-allocate circular history buffer (no audio-thread allocation)
    mHistoryBuf.resize(static_cast<size_t>(kMaxHistoryBlocks), 0.0);
    mHistoryHead.store(0, std::memory_order_relaxed);
    mHistorySize.store(0, std::memory_order_relaxed);

    // Pre-allocate working buffers for updateIntegratedAndLRA
    mWindowPowers.assign(static_cast<size_t>(kMaxHistoryBlocks), 0.0);
    mPrefixSums.resize(static_cast<size_t>(kMaxHistoryBlocks + 1), 0.0);

    // Reset LRA histogram
    mLraHisto.fill(0);

    mUpdateCounter = 0;

    setupKWeightingFilters();
}

// ---------------------------------------------------------------------------
// K-weighting filter coefficients
// Derived from libebur128 / ITU-R BS.1770-4 Annex 2, valid for any sample rate.
// ---------------------------------------------------------------------------
void LoudnessMeter::setupKWeightingFilters()
{
    preFilters.assign(mNumChannels, Biquad{});
    rlbFilters.assign(mNumChannels, Biquad{});

    // ------------------------------------------------------------------
    // Stage 1: Pre-filter (high-shelf, +4 dB above ~1681 Hz)
    // Analog prototype: H1(s) = (Vh + Vb/Q*s + s^2) / (1 + 1/Q*s + s^2)
    // Bilinear transform with K = tan(pi * f0 / fs), f0 = 1681.974 Hz
    // ------------------------------------------------------------------
    {
        const double f0 = kBS1770PreFilterFreqHz;
        const double Q  = 0.7071752369554196;
        const double dB = 3.99984385397;

        const double Vh = std::pow(10.0, dB / 20.0);
        const double Vb = std::pow(Vh, 0.4996667741545416);

        const double K  = std::tan(kPi * f0 / mSampleRate);
        const double K2 = K * K;
        const double a0 = 1.0 + K / Q + K2;

        Biquad pre;
        pre.b0 = (Vh + Vb * K / Q + K2) / a0;
        pre.b1 = 2.0 * (K2 - Vh) / a0;
        pre.b2 = (Vh - Vb * K / Q + K2) / a0;
        pre.a1 = 2.0 * (K2 - 1.0) / a0;
        pre.a2 = (1.0 - K / Q + K2) / a0;

        for (auto& f : preFilters)
            f = pre;
    }

    // ------------------------------------------------------------------
    // Stage 2: RLB high-pass (Q from ITU-R BS.1770-4 Annex 2 / libebur128)
    // ------------------------------------------------------------------
    {
        const double fc = kBS1770RLBHighPassFreqHz;
        const double Q  = 0.5003270373238773; // BS.1770-4 Annex 2
        const double K  = std::tan(kPi * fc / mSampleRate);
        const double K2 = K * K;
        const double a0 = 1.0 + K / Q + K2;

        Biquad rlb;
        rlb.b0 =  1.0 / a0;
        rlb.b1 = -2.0 / a0;
        rlb.b2 =  1.0 / a0;
        rlb.a1 = 2.0 * (K2 - 1.0) / a0;
        rlb.a2 = (1.0 - K / Q + K2) / a0;

        for (auto& f : rlbFilters)
            f = rlb;
    }
}

// ---------------------------------------------------------------------------
// processBlock
// ---------------------------------------------------------------------------
void LoudnessMeter::processBlock(const juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh  = std::min(buffer.getNumChannels(), mNumChannels);
    const int numSmp = buffer.getNumSamples();

    for (int i = 0; i < numSmp; ++i)
    {
        // K-weight each channel and accumulate mean square
        double samplePower = 0.0;
        for (int ch = 0; ch < numCh; ++ch)
        {
            float s = buffer.getSample(ch, i);
            s = preFilters[ch].process(s);
            s = rlbFilters[ch].process(s);
            samplePower += static_cast<double>(s) * s;
        }
        // BS.1770-4 channel weighting: equal for L/R (G_i = 1.0)
        // Surround channels (LFE excluded) use G = 1.41; simplified here for stereo
        mBlockPower += samplePower;

        ++mBlockAccum;
        if (mBlockAccum >= mBlockSize)
        {
            // Finish this 100 ms block.
            // BS.1770-4: mean-square per channel is summed (not averaged) across channels.
            // mBlockPower = Σ_channels Σ_samples x^2; divide by mBlockSize to get
            // the channel-summed mean square z = Σ_i (1/T * Σ x_i^2).
            double blockMeanSquare = mBlockPower / static_cast<double>(mBlockSize);
            mBlockPower  = 0.0;
            mBlockAccum  = 0;
            onBlockComplete(blockMeanSquare);
        }
    }
}

// ---------------------------------------------------------------------------
// onBlockComplete — called after every 100 ms block
// ---------------------------------------------------------------------------
void LoudnessMeter::onBlockComplete(double blockMeanSquare)
{
    // --- Momentary (400 ms = 4 blocks) ---
    mMomentaryRing.push(blockMeanSquare);

    // --- Short-term (3 s = 30 blocks) ---
    mShortTermRing.push(blockMeanSquare);

    // --- Update momentary LUFS (requires full 400 ms window) ---
    if (mMomentaryRing.full())
        mMomentaryLUFS.store(powerToLUFS(mMomentaryRing.mean()));

    // --- Update short-term LUFS (requires full 3 s window) ---
    if (mShortTermRing.full())
        mShortTermLUFS.store(powerToLUFS(mShortTermRing.mean()));

    // --- Append to circular history buffer (no allocation) ---
    pushHistory(blockMeanSquare);

    // --- Update integrated+LRA (throttled to every kUpdateFreq blocks ≈ 1 s) ---
    ++mUpdateCounter;
    if (mUpdateCounter >= kUpdateFreq)
    {
        mUpdateCounter = 0;
        updateIntegratedAndLRA();
    }
}

// ---------------------------------------------------------------------------
// updateIntegratedAndLRA — thin coordinator: build prefix sums, then delegate.
// ---------------------------------------------------------------------------
void LoudnessMeter::updateIntegratedAndLRA()
{
    const int n = mHistorySize.load(std::memory_order_relaxed);
    if (n < kMomentaryBlocks)
        return;

    // Build prefix sums over the circular history (O(n)).
    // mPrefixSums is pre-allocated to kMaxHistoryBlocks+1 in prepare().
    mPrefixSums[0] = 0.0;
    for (int i = 0; i < n; ++i)
        mPrefixSums[static_cast<size_t>(i + 1)] = mPrefixSums[static_cast<size_t>(i)] + historyAt(i);

    mIntegratedLUFS.store(computeIntegratedLUFS());
    mLoudnessRange.store(computeLRA());
}

// ---------------------------------------------------------------------------
// computeIntegratedLUFS — two-pass gated loudness algorithm (BS.1770-4).
// Requires mPrefixSums to be populated by updateIntegratedAndLRA().
// ---------------------------------------------------------------------------
float LoudnessMeter::computeIntegratedLUFS()
{
    const int n = mHistorySize.load(std::memory_order_relaxed);

    // Build 400 ms sliding windows (hop = 100 ms).
    // mWindowPowers is pre-allocated to kMaxHistoryBlocks; use indexed assignment.
    const int numWin400 = n - kMomentaryBlocks + 1;
    for (int i = 0; i < numWin400; ++i)
    {
        const double sum = mPrefixSums[static_cast<size_t>(i + kMomentaryBlocks)]
                         - mPrefixSums[static_cast<size_t>(i)];
        mWindowPowers[static_cast<size_t>(i)] = sum / kMomentaryBlocks;
    }

    // Absolute gate: -70 LUFS
    const double absGateLinear = lufsToLinear(static_cast<float>(kAbsGateLUFS));

    // First pass: mean of all windows above absolute gate
    double sumAbove = 0.0;
    int    cntAbove = 0;
    for (int i = 0; i < numWin400; ++i)
    {
        const double p = mWindowPowers[static_cast<size_t>(i)];
        if (p > absGateLinear)
        {
            sumAbove += p;
            ++cntAbove;
        }
    }

    if (cntAbove == 0)
        return kNegInf;

    const double meanAbsGated  = sumAbove / cntAbove;
    const float  relGateLUFS   = powerToLUFS(meanAbsGated) + static_cast<float>(kRelGateOffset);
    const double relGateLinear = lufsToLinear(relGateLUFS);

    // Second pass: mean of all windows above relative gate
    double sumRel = 0.0;
    int    cntRel = 0;
    for (int i = 0; i < numWin400; ++i)
    {
        const double p = mWindowPowers[static_cast<size_t>(i)];
        if (p > absGateLinear && p > relGateLinear)
        {
            sumRel += p;
            ++cntRel;
        }
    }

    if (cntRel == 0)
        return kNegInf;

    return powerToLUFS(sumRel / cntRel);
}

// ---------------------------------------------------------------------------
// computeLRA — histogram-based loudness range (EBU R128 / BS.1770-4).
// Requires mPrefixSums to be populated by updateIntegratedAndLRA().
// ---------------------------------------------------------------------------
float LoudnessMeter::computeLRA()
{
    const int n = mHistorySize.load(std::memory_order_relaxed);

    if (n < kShortTermBlocks)
        return 0.0f;

    // Rebuild LRA histogram from current history (O(n)).
    mLraHisto.fill(0);
    int validCount = 0;

    const int numWin3s = n - kShortTermBlocks + 1;
    for (int i = 0; i < numWin3s; ++i)
    {
        const double sum = mPrefixSums[static_cast<size_t>(i + kShortTermBlocks)]
                         - mPrefixSums[static_cast<size_t>(i)];
        const float l = powerToLUFS(sum / kShortTermBlocks);

        // Gate: above absolute threshold (use a 1 LU margin for numerical safety)
        if (l > static_cast<float>(kAbsGateLUFS) - 1.0f)
        {
            int bin = static_cast<int>((l - kLraHistoMinLUFS) / kLraBinWidth);
            bin = std::clamp(bin, 0, kLraHistoBins - 1);
            ++mLraHisto[static_cast<size_t>(bin)];
            ++validCount;
        }
    }

    if (validCount < 2)
        return 0.0f;

    // Find 10th and 95th percentiles from histogram (O(kLraHistoBins) = O(900)).
    const int loTarget = static_cast<int>(0.10 * validCount);
    const int hiTarget = static_cast<int>(0.95 * validCount);

    int   cumul   = 0;
    float loLUFS  = kLraHistoMinLUFS;
    float hiLUFS  = kLraHistoMinLUFS;
    bool  foundLo = false;
    bool  foundHi = false;

    for (int b = 0; b < kLraHistoBins && !(foundLo && foundHi); ++b)
    {
        cumul += mLraHisto[static_cast<size_t>(b)];
        const float binLUFS = kLraHistoMinLUFS + static_cast<float>(b) * kLraBinWidth;

        if (!foundLo && cumul > loTarget)
        {
            loLUFS  = binLUFS;
            foundLo = true;
        }
        if (!foundHi && cumul >= hiTarget)
        {
            hiLUFS  = binLUFS;
            foundHi = true;
        }
    }

    if (foundLo && foundHi && hiLUFS > loLUFS)
        return hiLUFS - loLUFS;
    else
        return 0.0f;
}

// ---------------------------------------------------------------------------
// resetIntegrated
// ---------------------------------------------------------------------------
void LoudnessMeter::resetIntegrated()
{
    mHistoryHead.store(0, std::memory_order_relaxed);
    mHistorySize.store(0, std::memory_order_relaxed);
    mUpdateCounter = 0;
    mLraHisto.fill(0);
    mIntegratedLUFS.store(kNegInf);
    mLoudnessRange.store(0.0f);
}

// ---------------------------------------------------------------------------
// Getters — safe to call from any thread (atomic loads).
// ---------------------------------------------------------------------------
float LoudnessMeter::getMomentaryLUFS()  const { return mMomentaryLUFS.load();  }
float LoudnessMeter::getShortTermLUFS()  const { return mShortTermLUFS.load();  }
float LoudnessMeter::getIntegratedLUFS() const { return mIntegratedLUFS.load(); }
float LoudnessMeter::getLoudnessRange()  const { return mLoudnessRange.load();  }

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------
float LoudnessMeter::powerToLUFS(double power) noexcept
{
    if (power <= 0.0)
        return -std::numeric_limits<float>::infinity();
    return static_cast<float>(kLufsCorrectiondB + 10.0 * std::log10(power));
}

double LoudnessMeter::lufsToLinear(float lufs) noexcept
{
    return std::pow(10.0, (static_cast<double>(lufs) - kLufsCorrectiondB) / 10.0);
}
