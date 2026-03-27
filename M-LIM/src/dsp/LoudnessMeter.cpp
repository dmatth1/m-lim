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
static constexpr double kRelGateOffset   = -10.0;   // relative gate = ungated mean - 10 LU (integrated)
static constexpr double kLraRelGateOffset = -20.0;  // relative gate = ungated mean - 20 LU (LRA, EBU R128 §3.3)
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

        // Copy coefficients to stereo SIMD filter
        mPreFilter2.b0 = pre.b0; mPreFilter2.b1 = pre.b1; mPreFilter2.b2 = pre.b2;
        mPreFilter2.a1 = pre.a1; mPreFilter2.a2 = pre.a2;
        mPreFilter2.reset();
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

        // Copy coefficients to stereo SIMD filter
        mRlbFilter2.b0 = rlb.b0; mRlbFilter2.b1 = rlb.b1; mRlbFilter2.b2 = rlb.b2;
        mRlbFilter2.a1 = rlb.a1; mRlbFilter2.a2 = rlb.a2;
        mRlbFilter2.reset();
    }
}

// ---------------------------------------------------------------------------
// processBlock
// ---------------------------------------------------------------------------
/**
 * @brief Process one audio block and accumulate loudness metering data.
 *
 * @details K-weighting is applied to each channel sample via a two-stage biquad chain:
 *   1. **Pre-filter** (Stage 1): high-shelf boosting ~+4 dB above ~1682 Hz, modelling
 *      the acoustic effect of the head on the frontal sound field (BS.1770-4 Annex 2).
 *   2. **RLB weighting** (Stage 2): second-order high-pass at ~38 Hz, removing low-frequency
 *      content that has little contribution to perceived loudness (Revised Low-frequency B-curve).
 *
 * After K-weighting, the per-sample power is accumulated into 100 ms blocks:
 *   - mBlockPower accumulates the sum-of-squares across all channels and samples.
 *   - When mBlockAccum reaches mBlockSize (= round(fs × 0.1)), onBlockComplete() is called
 *     with the channel-summed mean-square for that block.
 *
 * BS.1770-4 channel weighting: G_i = 1.0 for L/R (stereo). Surround channels use G = 1.41,
 * but this implementation targets stereo only (no surround weighting applied).
 *
 * @note No heap allocation occurs. Real-time safe. Call from the audio thread only.
 * @see  ITU-R BS.1770-4, §4; EBU R128, §A.1
 */
void LoudnessMeter::processBlock(const juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh  = std::min(buffer.getNumChannels(), mNumChannels);
    const int numSmp = buffer.getNumSamples();

    if (numCh == 2)
    {
        // Stereo SIMD path: process L and R simultaneously via Biquad2.
        // On SSE2 platforms this computes both channels in parallel;
        // on non-SSE2 platforms Biquad2::processStereo falls through to scalar.
        for (int i = 0; i < numSmp; ++i)
        {
            const double xL = static_cast<double>(buffer.getSample(0, i));
            const double xR = static_cast<double>(buffer.getSample(1, i));
            double yL, yR;
            mPreFilter2.processStereo(xL, xR, yL, yR);
            mRlbFilter2.processStereo(yL, yR, yL, yR);
            mBlockPower += yL * yL + yR * yR;

            ++mBlockAccum;
            if (mBlockAccum >= mBlockSize)
            {
                double blockMeanSquare = mBlockPower / static_cast<double>(mBlockSize);
                mBlockPower = 0.0;
                mBlockAccum = 0;
                onBlockComplete(blockMeanSquare);
            }
        }
    }
    else
    {
        // Scalar fallback for mono and surround (numCh != 2).
        // BS.1770-4 channel weighting: G_i = 1.0 for L/R; simplified here for stereo.
        for (int i = 0; i < numSmp; ++i)
        {
            double samplePower = 0.0;
            for (int ch = 0; ch < numCh; ++ch)
            {
                float s = buffer.getSample(ch, i);
                s = preFilters[ch].process(s);
                s = rlbFilters[ch].process(s);
                samplePower += static_cast<double>(s) * s;
            }
            mBlockPower += samplePower;

            ++mBlockAccum;
            if (mBlockAccum >= mBlockSize)
            {
                double blockMeanSquare = mBlockPower / static_cast<double>(mBlockSize);
                mBlockPower  = 0.0;
                mBlockAccum  = 0;
                onBlockComplete(blockMeanSquare);
            }
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
/**
 * @brief Compute integrated loudness using the BS.1770-4 two-pass gating algorithm.
 *
 * @details ITU-R BS.1770-4 §4 / EBU R128 §3.2 define integrated loudness as the
 * mean-square power of audio that passes both an absolute and a relative gate:
 *
 *   **Pass 1 — absolute gate (-70 LUFS)**:
 *     Collect all 400 ms sliding windows (hop = 100 ms) whose mean-square exceeds
 *     the linear equivalent of -70 LUFS. Compute their mean power (M).
 *
 *   **Pass 2 — relative gate (M − 10 LU)**:
 *     From the pass-1 windows, keep only those also above M − 10 LU (the "relative gate").
 *     The integrated LUFS is computed from the mean of these double-gated windows.
 *
 * Both passes use the pre-built mPrefixSums array (O(1) window sum lookup) and
 * the pre-allocated mWindowPowers buffer (no allocation per call).
 *
 * Returns kNegInf if no windows survive either gate (silence or very short content).
 *
 * @see ITU-R BS.1770-4, §4; EBU R128, §3.2
 */
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
/**
 * @brief Compute loudness range (LRA) per EBU R128 §3.3 / ITU-R BS.1770-4.
 *
 * @details Loudness Range measures the variation of loudness over time, excluding
 * the extremes of the distribution. The algorithm:
 *
 *   1. Build 3 s sliding windows (hop = 100 ms) from the pre-built prefix sums.
 *   2. Gate windows below -70 LUFS (absolute gate per EBU R128 §4.6).
 *   3. Populate a fixed 900-bin histogram covering [-70, +20) LUFS at 0.1 LU resolution.
 *      This replaces a sort-based approach: finding percentiles is O(900) regardless of
 *      history length, avoiding O(n log n) cost on long sessions.
 *   4. Find the 10th percentile (L_lo) and 95th percentile (L_hi) by scanning the histogram.
 *   5. LRA = L_hi − L_lo (in LU).
 *
 * Returns 0.0 if fewer than kShortTermBlocks (30) history blocks are available
 * or if fewer than 2 valid windows exist.
 *
 * @see EBU R128, §3.3; ITU-R BS.1770-4, Annex 3
 */
float LoudnessMeter::computeLRA()
{
    const int n = mHistorySize.load(std::memory_order_relaxed);

    if (n < kShortTermBlocks)
        return 0.0f;

    // Two-pass gating per EBU R128 §3.3 / EBU Tech 3342 §4.4.
    const int numWin3s = n - kShortTermBlocks + 1;

    // Pass 1: compute the ungated mean power of windows above the absolute gate (-70 LUFS).
    // Use mWindowPowers as scratch storage (pre-allocated, no heap alloc).
    const double absGateLinear = lufsToLinear(static_cast<float>(kAbsGateLUFS));
    double sumAbsGated = 0.0;
    int    cntAbsGated = 0;

    for (int i = 0; i < numWin3s; ++i)
    {
        const double sum = mPrefixSums[static_cast<size_t>(i + kShortTermBlocks)]
                         - mPrefixSums[static_cast<size_t>(i)];
        const double power = sum / kShortTermBlocks;
        mWindowPowers[static_cast<size_t>(i)] = power;
        if (power > absGateLinear)
        {
            sumAbsGated += power;
            ++cntAbsGated;
        }
    }

    // Derive the relative gate: ungated mean - 20 LU (EBU R128 §3.3).
    // If nothing passed the absolute gate, return 0 (no measurable range).
    double relGateLinear = 0.0;
    if (cntAbsGated > 0)
    {
        const float ungatedMeanLUFS = powerToLUFS(sumAbsGated / cntAbsGated);
        relGateLinear = lufsToLinear(ungatedMeanLUFS + static_cast<float>(kLraRelGateOffset));
    }
    else
    {
        return 0.0f;
    }

    // Pass 2: populate the histogram with windows that pass BOTH gates.
    mLraHisto.fill(0);
    int validCount = 0;

    for (int i = 0; i < numWin3s; ++i)
    {
        const double power = mWindowPowers[static_cast<size_t>(i)];
        if (power > absGateLinear && power > relGateLinear)
        {
            const float l = powerToLUFS(power);
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

        if (!foundLo && cumul >= loTarget)
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
