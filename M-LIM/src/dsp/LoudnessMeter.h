#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>
#include <atomic>
#include <limits>

// SSE2 is available on all x86-64 targets and on x86 targets with /arch:SSE2 or -msse2.
#if defined(__SSE2__) || (defined(_MSC_VER) && (defined(_M_AMD64) || defined(_M_X64) \
    || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)))
#  include <emmintrin.h>
#  define MLIM_HAS_SSE2 1
#else
#  define MLIM_HAS_SSE2 0
#endif

/**
 * LoudnessMeter — ITU-R BS.1770-4 compliant loudness metering.
 *
 * Measures momentary (400 ms), short-term (3 s), and integrated LUFS,
 * plus loudness range (LRA) as defined in EBU R128 / BS.1770-4.
 *
 * K-weighting is applied in two stages:
 *   Stage 1: pre-filter (high-shelf ~+4 dB above ~1.5 kHz)
 *   Stage 2: RLB weighting (2nd-order high-pass at ~38 Hz)
 *
 * Internal processing uses 100 ms analysis blocks. Momentary uses the last
 * 4 blocks (400 ms), short-term uses the last 30 blocks (3 s).
 *
 * Real-time safety:
 *   - No heap allocations in processBlock() or its call chain.
 *   - Result fields are std::atomic<float> — safe to read from UI thread.
 *   - History is capped at kMaxHistoryBlocks (≈10 min) to bound memory & CPU.
 *   - Integrated/LRA recomputed every kUpdateFreq blocks (~1 s) to reduce load.
 */
class LoudnessMeter
{
public:
    LoudnessMeter() = default;

    /** Prepare the meter for a given sample rate and channel count.
     *  Must be called before processBlock(). */
    void prepare(double sampleRate, int numChannels);

    /** Process one block of audio. Call from the audio thread. */
    void processBlock(const juce::AudioBuffer<float>& buffer);

    /** Momentary loudness (400 ms window), in LUFS.
     *  Returns -infinity when the window is silent. */
    float getMomentaryLUFS() const;

    /** Short-term loudness (3 s window), in LUFS.
     *  Returns -infinity when the window is silent. */
    float getShortTermLUFS() const;

    /** Integrated loudness since last resetIntegrated() call, in LUFS.
     *  Returns -infinity before enough data has been accumulated. */
    float getIntegratedLUFS() const;

    /** Loudness range (EBU R128), in LU. */
    float getLoudnessRange() const;

    /** Reset the integrated loudness and LRA accumulation. */
    void resetIntegrated();

private:
    // -----------------------------------------------------------------------
    // Internal biquad filter (direct form II transposed)
    // -----------------------------------------------------------------------
    struct Biquad
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0;
        double z1 = 0.0, z2 = 0.0;

        float process(float x) noexcept
        {
            constexpr double kDenormalFix = 1e-25; // above double denormal, below -500 dBFS
            double xd = static_cast<double>(x);
            double y  = b0 * xd + z1;
            z1 = b1 * xd - a1 * y + z2 + kDenormalFix;
            z2 = b2 * xd - a2 * y - kDenormalFix;
            return static_cast<float>(y);
        }

        void reset() noexcept { z1 = z2 = 0.0; }
    };

    // -----------------------------------------------------------------------
    // Stereo SIMD biquad: processes L and R in a single SSE2 operation.
    // Coefficients are the same for both channels (see setupKWeightingFilters).
    // State arrays are 16-byte aligned so __m128d loads/stores are aligned.
    // -----------------------------------------------------------------------
    struct alignas(16) Biquad2
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0;
        alignas(16) double z1[2] = {0.0, 0.0}; // z1 for [L, R]
        alignas(16) double z2[2] = {0.0, 0.0}; // z2 for [L, R]

        /** Process a stereo sample pair. Coefficients must be identical for both channels. */
        void processStereo(double xL, double xR, double& yL, double& yR) noexcept
        {
            constexpr double kFix = 1e-25;
#if MLIM_HAS_SSE2
            const __m128d vX      = _mm_set_pd(xR, xL);        // lo=L, hi=R
            const __m128d vZ1     = _mm_load_pd(z1);
            const __m128d vZ2     = _mm_load_pd(z2);
            const __m128d vB0     = _mm_set1_pd(b0);
            const __m128d vB1     = _mm_set1_pd(b1);
            const __m128d vB2     = _mm_set1_pd(b2);
            const __m128d vA1     = _mm_set1_pd(a1);
            const __m128d vA2     = _mm_set1_pd(a2);
            const __m128d vFix    = _mm_set1_pd( kFix);
            const __m128d vNegFix = _mm_set1_pd(-kFix);

            // Direct form II transposed: y = b0*x + z1
            const __m128d vY = _mm_add_pd(_mm_mul_pd(vB0, vX), vZ1);

            // z1_new = b1*x - a1*y + z2 + kFix
            _mm_store_pd(z1, _mm_add_pd(
                _mm_add_pd(_mm_sub_pd(_mm_mul_pd(vB1, vX), _mm_mul_pd(vA1, vY)), vZ2),
                vFix));

            // z2_new = b2*x - a2*y - kFix
            _mm_store_pd(z2, _mm_add_pd(
                _mm_sub_pd(_mm_mul_pd(vB2, vX), _mm_mul_pd(vA2, vY)),
                vNegFix));

            yL = _mm_cvtsd_f64(vY);
            yR = _mm_cvtsd_f64(_mm_unpackhi_pd(vY, vY));
#else
            // Scalar fallback (non-SSE2 platforms)
            yL  = b0 * xL + z1[0];
            z1[0] = b1 * xL - a1 * yL + z2[0] + kFix;
            z2[0] = b2 * xL - a2 * yL - kFix;
            yR  = b0 * xR + z1[1];
            z1[1] = b1 * xR - a1 * yR + z2[1] + kFix;
            z2[1] = b2 * xR - a2 * yR - kFix;
#endif
        }

        void reset() noexcept { z1[0] = z1[1] = z2[0] = z2[1] = 0.0; }
    };

    // K-weighting filter pair per channel
    std::vector<Biquad> preFilters;   // Stage 1: high-shelf pre-filter (scalar/non-stereo path)
    std::vector<Biquad> rlbFilters;   // Stage 2: RLB high-pass (scalar/non-stereo path)

    // Stereo SIMD filter pair (used when numCh == 2)
    Biquad2 mPreFilter2;
    Biquad2 mRlbFilter2;

    double mSampleRate  = 44100.0;
    int    mNumChannels = 2;

    // -----------------------------------------------------------------------
    // 100 ms block accumulation
    // -----------------------------------------------------------------------
    int    mBlockSize   = 0;   // samples per 100 ms block at current sample rate
    int    mBlockAccum  = 0;   // sample counter within current block
    double mBlockPower  = 0.0; // accumulated sum-of-squares for current block (all channels)

    // Ring buffers of block mean-square values (bounded: 4 and 30 entries)
    static constexpr int kMomentaryBlocks  = 4;
    static constexpr int kShortTermBlocks  = 30;

    /** Fixed-capacity ring buffer with incremental running sum.
     *  No heap allocation after construction — all storage is inline.
     *  push() is O(1); sum()/mean() are O(1) (maintained incrementally). */
    template<int N>
    struct FixedRingBuffer
    {
        std::array<double, N> buf{};
        int    head    = 0;    ///< index of oldest element
        int    count   = 0;    ///< number of valid elements (0..N)
        double runSum  = 0.0;  ///< maintained incrementally

        void push(double val) noexcept
        {
            if (count == N)
            {
                runSum    -= buf[static_cast<size_t>(head)];
                buf[static_cast<size_t>(head)] = val;
                head       = (head + 1) % N;
            }
            else
            {
                buf[static_cast<size_t>((head + count) % N)] = val;
                ++count;
            }
            runSum += val;
        }

        double sum()  const noexcept { return runSum; }
        double mean() const noexcept { return count > 0 ? runSum / count : 0.0; }
        bool   full() const noexcept { return count == N; }

        void reset() noexcept
        {
            buf.fill(0.0);
            head   = 0;
            count  = 0;
            runSum = 0.0;
        }
    };

    FixedRingBuffer<kMomentaryBlocks> mMomentaryRing;
    FixedRingBuffer<kShortTermBlocks> mShortTermRing;

    // -----------------------------------------------------------------------
    // Bounded circular history buffer (replaces unbounded mGatedBlockHistory).
    // Caps memory and per-update computation to ≈10 minutes of audio.
    // -----------------------------------------------------------------------
    static constexpr int kMaxHistoryBlocks = 6000; // 10 min at 100ms/block

    std::vector<double>    mHistoryBuf;           // pre-allocated ring buffer (size = kMaxHistoryBlocks)
    std::atomic<int>       mHistoryHead {0};      // index of oldest element
    std::atomic<int>       mHistorySize {0};       // valid element count (0..kMaxHistoryBlocks)

    // Pre-allocated working buffers — filled in updateIntegratedAndLRA() without alloc.
    std::vector<double> mWindowPowers;     // 400 ms window mean-squares (used by computeIntegratedLUFS)
    std::vector<double> mLraWindowPowers;  // 3 s window mean-squares (used by computeLRA)
    std::vector<double> mPrefixSums;       // prefix sums over history (size = kMaxHistoryBlocks+1)

    // -----------------------------------------------------------------------
    // LRA histogram: 900 bins covering [-70, +20) LUFS at 0.1 LU resolution.
    // Replaces std::sort — finding percentiles is O(900) per update.
    // -----------------------------------------------------------------------
    static constexpr int   kLraHistoBins    = 900;
    static constexpr float kLraHistoMinLUFS = -70.0f;
    static constexpr float kLraBinWidth     = 0.1f;

    std::array<int, kLraHistoBins> mLraHisto{};

    // -----------------------------------------------------------------------
    // Update throttle — integrated+LRA recomputed every kUpdateFreq blocks
    // (≈1 s) rather than on every 100 ms block.
    // -----------------------------------------------------------------------
    static constexpr int kUpdateFreq    = 10;
    int                  mUpdateCounter = 0;

    // -----------------------------------------------------------------------
    // Results — written on audio thread, read on UI thread via atomics.
    // -----------------------------------------------------------------------
    static constexpr float kNegInf = -std::numeric_limits<float>::infinity();

    std::atomic<float> mMomentaryLUFS  {kNegInf};
    std::atomic<float> mShortTermLUFS  {kNegInf};
    std::atomic<float> mIntegratedLUFS {kNegInf};
    std::atomic<float> mLoudnessRange  {0.0f};

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------
    void setupKWeightingFilters();
    void onBlockComplete(double blockMeanSquare);
    void updateIntegratedAndLRA();

    float computeIntegratedLUFS();
    float computeLRA();

    /** Append a block value to the circular history buffer. Lock-free, no alloc. */
    void pushHistory(double val) noexcept
    {
        const int head = mHistoryHead.load(std::memory_order_relaxed);
        const int size = mHistorySize.load(std::memory_order_relaxed);
        const int writeIdx = (head + size) % kMaxHistoryBlocks;
        mHistoryBuf[static_cast<size_t>(writeIdx)] = val;
        if (size < kMaxHistoryBlocks)
            mHistorySize.store(size + 1, std::memory_order_relaxed);
        else
            mHistoryHead.store((head + 1) % kMaxHistoryBlocks, std::memory_order_relaxed);
    }

    /** Access the i-th history element (0 = oldest). */
    double historyAt(int i) const noexcept
    {
        return mHistoryBuf[static_cast<size_t>(
            (mHistoryHead.load(std::memory_order_relaxed) + i) % kMaxHistoryBlocks)];
    }

    /** Convert mean-square power (linear) to LUFS.
     *  LUFS = -0.691 + 10 * log10(power). */
    static float powerToLUFS(double power) noexcept;

    /** LUFS to linear power. */
    static double lufsToLinear(float lufs) noexcept;
};
