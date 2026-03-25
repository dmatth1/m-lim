#include "TruePeakDetector.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

// ITU-R BS.1770-4 Table 1 polyphase FIR coefficients — 4 phases x 12 taps
// Phase 0 and Phase 3 are mirrors; Phase 1 and Phase 2 are mirrors.
const float TruePeakDetector::kCoeffs[TruePeakDetector::kPhases][TruePeakDetector::kFirTaps] = {
    // Phase 0 (near-identity, main lobe at tap 6)
    {  0.0017089843750f,  0.0109863281250f, -0.0196533203125f,  0.0332031250000f,
      -0.0594482421875f,  0.1373291015625f,  0.9721679687500f, -0.1022949218750f,
       0.0476074218750f, -0.0266113281250f,  0.0148925781250f, -0.0083007812500f },
    // Phase 1 (1/4 sample offset)
    { -0.0291748046875f,  0.0292968750000f, -0.0517578125000f,  0.0891113281250f,
      -0.1665039062500f,  0.4650878906250f,  0.7797851562500f, -0.2003173828125f,
       0.1015625000000f, -0.0582275390625f,  0.0330810546875f, -0.0189208984375f },
    // Phase 2 (2/4 sample offset, mirror of Phase 1)
    { -0.0189208984375f,  0.0330810546875f, -0.0582275390625f,  0.1015625000000f,
      -0.2003173828125f,  0.7797851562500f,  0.4650878906250f, -0.1665039062500f,
       0.0891113281250f, -0.0517578125000f,  0.0292968750000f, -0.0291748046875f },
    // Phase 3 (3/4 sample offset, mirror of Phase 0)
    { -0.0083007812500f,  0.0148925781250f, -0.0266113281250f,  0.0476074218750f,
      -0.1022949218750f,  0.9721679687500f,  0.1373291015625f, -0.0594482421875f,
       0.0332031250000f, -0.0196533203125f,  0.0109863281250f,  0.0017089843750f }
};

// Tap-major transposed layout: kCoeffsByTap[tap][phase] = kCoeffs[phase][tap]
// Allows loading all 4 phase coefficients for a given tap in one SIMD load.
const float TruePeakDetector::kCoeffsByTap[TruePeakDetector::kFirTaps][TruePeakDetector::kPhases] = {
    { kCoeffs[0][0],  kCoeffs[1][0],  kCoeffs[2][0],  kCoeffs[3][0]  },  // tap 0
    { kCoeffs[0][1],  kCoeffs[1][1],  kCoeffs[2][1],  kCoeffs[3][1]  },  // tap 1
    { kCoeffs[0][2],  kCoeffs[1][2],  kCoeffs[2][2],  kCoeffs[3][2]  },  // tap 2
    { kCoeffs[0][3],  kCoeffs[1][3],  kCoeffs[2][3],  kCoeffs[3][3]  },  // tap 3
    { kCoeffs[0][4],  kCoeffs[1][4],  kCoeffs[2][4],  kCoeffs[3][4]  },  // tap 4
    { kCoeffs[0][5],  kCoeffs[1][5],  kCoeffs[2][5],  kCoeffs[3][5]  },  // tap 5
    { kCoeffs[0][6],  kCoeffs[1][6],  kCoeffs[2][6],  kCoeffs[3][6]  },  // tap 6
    { kCoeffs[0][7],  kCoeffs[1][7],  kCoeffs[2][7],  kCoeffs[3][7]  },  // tap 7
    { kCoeffs[0][8],  kCoeffs[1][8],  kCoeffs[2][8],  kCoeffs[3][8]  },  // tap 8
    { kCoeffs[0][9],  kCoeffs[1][9],  kCoeffs[2][9],  kCoeffs[3][9]  },  // tap 9
    { kCoeffs[0][10], kCoeffs[1][10], kCoeffs[2][10], kCoeffs[3][10] },  // tap 10
    { kCoeffs[0][11], kCoeffs[1][11], kCoeffs[2][11], kCoeffs[3][11] },  // tap 11
};

void TruePeakDetector::prepare(double sampleRate)
{
    mSampleRate = sampleRate;
    reset();
}

float TruePeakDetector::processSample(float sample)
{
    // Update linear staging buffer — write at both halves to avoid modulo in FIR loop.
    // mLinearPos always points to the slot for the oldest sample after the increment.
    mLinearBuf[static_cast<size_t>(mLinearPos)]            = sample;
    mLinearBuf[static_cast<size_t>(mLinearPos + kFirTaps)] = sample;
    mLinearPos = (mLinearPos + 1) % kFirTaps;

    // src points to the oldest sample in the linear buffer; FIR reads kFirTaps
    // contiguous floats with no modulo in the inner loop.
    const float* src = &mLinearBuf[static_cast<size_t>(mLinearPos)];

    float maxAbs = 0.0f;

#if JUCE_USE_SIMD
    // SIMD path: compute all 4 phase FIR outputs in parallel using tap-major table.
    // Only active when the native SIMD register holds exactly 4 floats (SSE/NEON 128-bit).
    // On wider SIMD (e.g. AVX 256-bit = 8 floats), fall through to the scalar path to
    // avoid reading past the end of kCoeffsByTap[tap][4].
    {
        using SIMDf = juce::dsp::SIMDRegister<float>;
        if constexpr (SIMDf::SIMDNumElements == 4)
        {
            SIMDf acc = SIMDf::expand(0.0f);
            for (int tap = 0; tap < kFirTaps; ++tap)
            {
                auto coeffs = SIMDf::fromRawArray(kCoeffsByTap[tap]);
                auto s      = SIMDf::expand(src[tap]);   // broadcast scalar to all lanes
                acc          = acc + coeffs * s;
            }

            float phaseOutputs[4];
            acc.copyToRawArray(phaseOutputs);

            for (int p = 0; p < kPhases; ++p)
            {
                float absVal = std::abs(phaseOutputs[p]);
                if (absVal > maxAbs)
                    maxAbs = absVal;
            }
        }
        else
        {
            // Scalar fallback for wide SIMD (AVX etc.)
            for (int phase = 0; phase < kPhases; ++phase)
            {
                float sum = 0.0f;
                for (int tap = 0; tap < kFirTaps; ++tap)
                    sum += kCoeffsByTap[tap][phase] * src[tap];
                float absVal = std::abs(sum);
                if (absVal > maxAbs)
                    maxAbs = absVal;
            }
        }
    }
#else
    // Scalar fallback: same computation using tap-major table and linear buffer
    for (int phase = 0; phase < kPhases; ++phase)
    {
        float sum = 0.0f;
        for (int tap = 0; tap < kFirTaps; ++tap)
            sum += kCoeffsByTap[tap][phase] * src[tap];
        float absVal = std::abs(sum);
        if (absVal > maxAbs)
            maxAbs = absVal;
    }
#endif

    if (maxAbs > mPeak.load(std::memory_order_relaxed))
        mPeak.store(maxAbs, std::memory_order_relaxed);

    return maxAbs;
}

float TruePeakDetector::processSampleScalar(float sample)
{
    // Pure scalar path using the original phase-major kCoeffs table and circular buffer.
    // Intended for use on a separate TruePeakDetector instance in parity tests.

    // Write into circular buffer
    mBuffer[static_cast<size_t>(mWritePos)] = sample;
    mWritePos = (mWritePos + 1) % kFirTaps;

    float maxAbs = 0.0f;
    for (int phase = 0; phase < kPhases; ++phase)
    {
        float sum = 0.0f;
        for (int tap = 0; tap < kFirTaps; ++tap)
        {
            int idx = (mWritePos - 1 - tap + kFirTaps) % kFirTaps;
            sum += kCoeffs[phase][tap] * mBuffer[static_cast<size_t>(idx)];
        }
        float absVal = std::abs(sum);
        if (absVal > maxAbs)
            maxAbs = absVal;
    }

    if (maxAbs > mPeak.load(std::memory_order_relaxed))
        mPeak.store(maxAbs, std::memory_order_relaxed);

    return maxAbs;
}

void TruePeakDetector::processBlock(const float* input, int numSamples)
{
    juce::ScopedNoDenormals noDenormals;
    for (int i = 0; i < numSamples; ++i)
        processSample(input[i]);
}

float TruePeakDetector::getPeak() const
{
    return mPeak.load(std::memory_order_relaxed);
}

void TruePeakDetector::reset()
{
    mBuffer.fill(0.0f);
    mWritePos = 0;
    mLinearBuf.fill(0.0f);
    mLinearPos = 0;
    mPeak.store(0.0f, std::memory_order_relaxed);
}
