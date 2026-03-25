#include "LoudnessMeter.h"

#include <cmath>
#include <algorithm>
#include <numeric>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
static constexpr double kPi     = 3.14159265358979323846;
static constexpr double kLufsCorrectiondB = -0.691; // BS.1770-4: 10*log10(Σ G_i * z_i) - 0.691
static constexpr double kAbsGateLUFS      = -70.0;  // absolute gate threshold
static constexpr double kRelGateOffset    = -10.0;  // relative gate = ungated mean - 10 LU

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

    mMomentaryBuffer.clear();
    mShortTermBuffer.clear();

    mMomentaryLUFS  = -std::numeric_limits<float>::infinity();
    mShortTermLUFS  = -std::numeric_limits<float>::infinity();
    mIntegratedLUFS = -std::numeric_limits<float>::infinity();
    mLoudnessRange  = 0.0f;

    resetIntegrated();
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
        const double f0 = 1681.974450955533;
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
    // Stage 2: RLB high-pass (2nd-order Butterworth, fc ~38.135 Hz)
    // ------------------------------------------------------------------
    {
        const double fc = 38.13547087602444;
        const double K  = std::tan(kPi * fc / mSampleRate);
        const double K2 = K * K;
        const double sq2 = std::sqrt(2.0);
        const double a0 = 1.0 + sq2 * K + K2;

        Biquad rlb;
        rlb.b0 =  1.0 / a0;
        rlb.b1 = -2.0 / a0;
        rlb.b2 =  1.0 / a0;
        rlb.a1 = 2.0 * (K2 - 1.0) / a0;
        rlb.a2 = (1.0 - sq2 * K + K2) / a0;

        for (auto& f : rlbFilters)
            f = rlb;
    }
}

// ---------------------------------------------------------------------------
// processBlock
// ---------------------------------------------------------------------------
void LoudnessMeter::processBlock(const juce::AudioBuffer<float>& buffer)
{
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
    mMomentaryBuffer.push_back(blockMeanSquare);
    if (static_cast<int>(mMomentaryBuffer.size()) > kMomentaryBlocks)
        mMomentaryBuffer.pop_front();

    // --- Short-term (3 s = 30 blocks) ---
    mShortTermBuffer.push_back(blockMeanSquare);
    if (static_cast<int>(mShortTermBuffer.size()) > kShortTermBlocks)
        mShortTermBuffer.pop_front();

    // --- Update momentary LUFS ---
    if (static_cast<int>(mMomentaryBuffer.size()) == kMomentaryBlocks)
    {
        double sum = 0.0;
        for (double v : mMomentaryBuffer)
            sum += v;
        mMomentaryLUFS = powerToLUFS(sum / kMomentaryBlocks);
    }

    // --- Update short-term LUFS ---
    {
        double sum = 0.0;
        for (double v : mShortTermBuffer)
            sum += v;
        mShortTermLUFS = powerToLUFS(sum / static_cast<double>(mShortTermBuffer.size()));
    }

    // --- Store block for integrated / LRA ---
    mGatedBlockHistory.push_back(blockMeanSquare);
    updateIntegratedAndLRA();
}

// ---------------------------------------------------------------------------
// updateIntegratedAndLRA
// ---------------------------------------------------------------------------
void LoudnessMeter::updateIntegratedAndLRA()
{
    // Integrated LUFS uses 400 ms blocks (4 × 100 ms) with 75 % overlap.
    // We treat each 100 ms slot as a potential start of a 400 ms window.
    // Minimum: need at least 4 blocks to form the first window.
    const int n = static_cast<int>(mGatedBlockHistory.size());
    if (n < kMomentaryBlocks)
        return;

    // Build list of 400 ms window mean powers (hop = 1 block = 100 ms)
    std::vector<double> windowPowers;
    windowPowers.reserve(static_cast<size_t>(n - kMomentaryBlocks + 1));

    for (int i = 0; i <= n - kMomentaryBlocks; ++i)
    {
        double sum = 0.0;
        for (int k = i; k < i + kMomentaryBlocks; ++k)
            sum += mGatedBlockHistory[static_cast<size_t>(k)];
        windowPowers.push_back(sum / kMomentaryBlocks);
    }

    // Absolute gate: -70 LUFS
    const double absGateLinear = lufsToLinear(static_cast<float>(kAbsGateLUFS));

    // First pass: mean of all windows above absolute gate
    double sumAbove = 0.0;
    int    cntAbove = 0;
    for (double p : windowPowers)
    {
        if (p > absGateLinear)
        {
            sumAbove += p;
            ++cntAbove;
        }
    }

    if (cntAbove == 0)
    {
        mIntegratedLUFS = -std::numeric_limits<float>::infinity();
        return;
    }

    const double meanAbsGated  = sumAbove / cntAbove;
    const float  relGateLUFS   = powerToLUFS(meanAbsGated) + static_cast<float>(kRelGateOffset);
    const double relGateLinear = lufsToLinear(relGateLUFS);

    // Second pass: mean of all windows above relative gate
    double sumRel = 0.0;
    int    cntRel = 0;
    for (double p : windowPowers)
    {
        if (p > absGateLinear && p > relGateLinear)
        {
            sumRel += p;
            ++cntRel;
        }
    }

    if (cntRel == 0)
    {
        mIntegratedLUFS = -std::numeric_limits<float>::infinity();
        return;
    }

    mIntegratedLUFS = powerToLUFS(sumRel / cntRel);

    // -----------------------------------------------------------------
    // LRA: uses short-term values above the absolute gate
    // Collect short-term LUFS for each overlapping 3 s window
    // -----------------------------------------------------------------
    if (n < kShortTermBlocks)
    {
        mLoudnessRange = 0.0f;
        return;
    }

    std::vector<float> stLoudness;
    stLoudness.reserve(static_cast<size_t>(n - kShortTermBlocks + 1));

    for (int i = 0; i <= n - kShortTermBlocks; ++i)
    {
        double sum = 0.0;
        for (int k = i; k < i + kShortTermBlocks; ++k)
            sum += mGatedBlockHistory[static_cast<size_t>(k)];
        float l = powerToLUFS(sum / kShortTermBlocks);
        if (l > kAbsGateLUFS - 1.0f) // include anything above ~-71 LU
            stLoudness.push_back(l);
    }

    if (stLoudness.size() < 2)
    {
        mLoudnessRange = 0.0f;
        return;
    }

    std::sort(stLoudness.begin(), stLoudness.end());

    // 10th percentile → low, 95th percentile → high
    const size_t sz  = stLoudness.size();
    const size_t lo  = static_cast<size_t>(std::floor(0.10 * sz));
    const size_t hi  = static_cast<size_t>(std::min(
                           static_cast<size_t>(std::ceil(0.95 * sz)), sz - 1));

    if (hi > lo)
        mLoudnessRange = stLoudness[hi] - stLoudness[lo];
    else
        mLoudnessRange = 0.0f;
}

// ---------------------------------------------------------------------------
// resetIntegrated
// ---------------------------------------------------------------------------
void LoudnessMeter::resetIntegrated()
{
    mGatedBlockHistory.clear();
    mIntegratedLUFS = -std::numeric_limits<float>::infinity();
    mLoudnessRange  = 0.0f;
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------
float LoudnessMeter::getMomentaryLUFS()  const { return mMomentaryLUFS;  }
float LoudnessMeter::getShortTermLUFS()  const { return mShortTermLUFS;  }
float LoudnessMeter::getIntegratedLUFS() const { return mIntegratedLUFS; }
float LoudnessMeter::getLoudnessRange()  const { return mLoudnessRange;  }

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
