#pragma once
#include <atomic>
#include <cstring>
#include <vector>

/**
 * MeterData — snapshot of audio metering data from the DSP thread.
 *
 * Captured on the audio thread and pushed via LockFreeFIFO to the UI thread.
 * All fields are plain POD so the struct can be copied without locks.
 */
struct MeterData
{
    float inputLevelL  = 0.0f;   ///< Input peak level, left channel (linear, 0–1)
    float inputLevelR  = 0.0f;   ///< Input peak level, right channel
    float outputLevelL = 0.0f;   ///< Output peak level, left channel
    float outputLevelR = 0.0f;   ///< Output peak level, right channel
    float gainReduction = 0.0f;  ///< Gain reduction in dB (0 = none, negative = reducing)
    float truePeakL    = 0.0f;   ///< True peak level, left channel (linear)
    float truePeakR    = 0.0f;   ///< True peak level, right channel

    float momentaryLUFS  = 0.0f; ///< Momentary loudness (400 ms window), in LUFS
    float shortTermLUFS  = 0.0f; ///< Short-term loudness (3 s window), in LUFS
    float integratedLUFS = 0.0f; ///< Integrated loudness since reset, in LUFS
    float loudnessRange  = 0.0f; ///< Loudness range (EBU R128), in LU

    float waveformSample = 0.0f;  ///< GR sample for WaveformDisplay (one per audio block)
};

/**
 * LockFreeFIFO<T> — single-producer single-consumer lock-free ring buffer.
 *
 * Thread safety: push() may only be called from one thread (the audio thread),
 * pop() may only be called from one other thread (the UI thread).
 * The implementation uses a power-of-two capacity for efficient masking.
 */
template<typename T>
class LockFreeFIFO
{
public:
    explicit LockFreeFIFO(int capacity = 64)
        : mCapacity(nextPowerOfTwo(capacity))
        , mMask(mCapacity - 1)
        , mBuffer(static_cast<std::size_t>(mCapacity))
        , mWritePos(0)
        , mReadPos(0)
    {}

    /** Returns true if the FIFO is empty. */
    bool isEmpty() const noexcept
    {
        return mReadPos.load(std::memory_order_acquire)
            == mWritePos.load(std::memory_order_acquire);
    }

    /** Returns true if the FIFO is full. */
    bool isFull() const noexcept
    {
        int write = mWritePos.load(std::memory_order_acquire);
        int read  = mReadPos.load(std::memory_order_acquire);
        return ((write + 1) & mMask) == read;
    }

    // Invariant: mWritePos and mReadPos are always stored pre-masked to [0, mMask].

    /**
     * Push an item onto the FIFO. Returns false if full (item is dropped).
     * Call only from the producer thread.
     */
    bool push(const T& item) noexcept
    {
        const int write = mWritePos.load(std::memory_order_relaxed);
        const int next  = (write + 1) & mMask;
        if (next == mReadPos.load(std::memory_order_acquire))
            return false;  // full

        mBuffer[static_cast<std::size_t>(write)] = item;  // write is pre-masked
        mWritePos.store(next, std::memory_order_release);
        return true;
    }

    /**
     * Pop an item from the FIFO. Returns false if empty.
     * Call only from the consumer thread.
     */
    bool pop(T& item) noexcept
    {
        const int read = mReadPos.load(std::memory_order_relaxed);
        if (read == mWritePos.load(std::memory_order_acquire))
            return false;  // empty

        item = mBuffer[static_cast<std::size_t>(read)];  // read is pre-masked
        mReadPos.store((read + 1) & mMask, std::memory_order_release);
        return true;
    }

    int capacity() const noexcept { return mCapacity; }

private:
    static int nextPowerOfTwo(int n) noexcept
    {
        if (n <= 1) return 2;
        --n;
        n |= n >> 1; n |= n >> 2; n |= n >> 4; n |= n >> 8; n |= n >> 16;
        return n + 1;
    }

    const int mCapacity;
    const int mMask;
    std::vector<T> mBuffer;
    std::atomic<int> mWritePos;
    std::atomic<int> mReadPos;
};
