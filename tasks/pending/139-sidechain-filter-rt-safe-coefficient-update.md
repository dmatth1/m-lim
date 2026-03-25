# Task 139: SidechainFilter coefficient update allocates heap on audio thread

## Description
`SidechainFilter::updateCoefficients()` (called from `process()` on the audio thread when
`mCoeffsDirty` is set) calls the JUCE IIR factory methods:

```cpp
auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, freq);
auto lpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, freq);
auto lsCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, freq, q, gain);
auto hsCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, freq, q, gain);
```

Each factory method returns a `ReferenceCountedObjectPtr<Coefficients<float>>`, which
heap-allocates a new `Coefficients` object via `new`. Assigning the result to
`mHP[ch].coefficients = hpCoeffs` releases the old reference-counted object (potential
`delete`) and retains the new one. This is a real-time safety violation: any time the
user moves a sidechain filter knob, a heap alloc/free pair hits the audio thread on the
next block.

The fix is to pre-allocate the four `Coefficients` objects once in `prepare()` and
reuse them in `updateCoefficients()` by copying values in-place with `*existing = *new`.
Because `juce::dsp::IIR::Coefficients<float>` is a thin wrapper around a `std::array<float,
5>`, assigning `*mHPCoeffs = *Coefficients<float>::makeHighPass(sr, freq)` copies the
five biquad coefficients without freeing/allocating the outer wrapper. The factory method
*still* allocates a temporary, but that temporary can be created on a non-RT-critical
path or, more accurately, the assignment pattern avoids *freeing the live object* on the
audio thread (the temp goes out of scope on the audio thread, but no *live* audio-path
object is freed).

The truly RT-safe approach is to compute the biquad coefficients manually (inline, no
heap) in a helper and write them directly into `mHP[ch].coefficients->coefficients`:

```cpp
// Example for HP biquad via bilinear transform
static void applyButterworthHP(juce::dsp::IIR::Filter<float>& flt,
                                double sr, float cutHz) noexcept {
    const float K = std::tan(juce::MathConstants<float>::pi * cutHz / (float)sr);
    const float norm = 1.0f / (1.0f + juce::MathConstants<float>::sqrt2 * K + K * K);
    auto& c = flt.coefficients->coefficients;
    c[0] = norm;          // b0
    c[1] = -2.0f * norm;  // b1
    c[2] = norm;           // b2
    c[3] = 2.0f * (K*K - 1.0f) * norm;   // a1
    c[4] = (1.0f - juce::MathConstants<float>::sqrt2 * K + K*K) * norm; // a2
}
```

This writes directly into the existing coefficients array with no allocation.

Implement the in-place coefficient calculation in `SidechainFilter::updateCoefficients()`
for all four filter types (HP, LP, low-shelf, high-shelf), replacing the factory-method
calls. The `mHP[ch]`, `mLP[ch]`, `mTiltLS[ch]`, `mTiltHS[ch]` filter objects still need
their `.coefficients` ReferenceCountedObjectPtr to point to an allocated `Coefficients`
object — pre-allocate one per filter slot in `prepare()` and reuse it every call.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/SidechainFilter.h` — add pre-allocated coefficient member pointers
Modify: `src/dsp/SidechainFilter.cpp` — rewrite `updateCoefficients()` to use in-place
  writes; pre-allocate in `prepare()`
Read: `src/dsp/SidechainFilter.cpp` (full) — existing implementation to replace

## Acceptance Criteria
- [ ] Run: `grep -n "makeHighPass\|makeLowPass\|makeLowShelf\|makeHighShelf" src/dsp/SidechainFilter.cpp` → Expected: no matches (factory calls removed from the non-prepare path) OR all remaining matches are inside `prepare()` only
- [ ] Run: `cd build && ctest --output-on-failure -R MLIMTests` → Expected: all tests pass
- [ ] Run: `grep -n "updateCoefficients" src/dsp/SidechainFilter.cpp` → Expected: function body contains no `make*` calls, only arithmetic and direct coefficient writes

## Tests
- Unit: `tests/dsp/test_sidechain_filter.cpp` — add a test that calls `prepare()`, then
  calls `setHighPassFreq/setLowPassFreq/setTilt` and then `process()` repeatedly, verifying
  the filter output is consistent and no heap allocation occurs (if a heap-allocation tracer
  is available; otherwise verify correctness of filter output vs reference values at 1 kHz HP)

## Technical Details
- `juce::dsp::IIR::Filter<float>::coefficients` is `ReferenceCountedObjectPtr<Coefficients<float>>`
- `Coefficients<float>::coefficients` is `std::array<float, 6>` holding `[b0, b1, b2, a1, a2]`
  (normalised, a0=1 implicit)
- Pre-allocate via: `mHPCoeffs[ch] = new juce::dsp::IIR::Coefficients<float>()` in
  `prepare()`, then assign `mHP[ch].coefficients = mHPCoeffs[ch]`
- In `updateCoefficients()`, compute coefficients into a local `std::array<float, 5>` and
  write to `mHP[ch].coefficients->coefficients` directly (no new allocation)
- Second-order Butterworth HP/LP biquad coefficients via bilinear transform are
  well-documented and straightforward to compute inline

## Dependencies
None
