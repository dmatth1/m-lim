# Task 477: SIMD Optimization — LoudnessMeter Power Accumulation in Stereo Path

## Description
The `LoudnessMeter::processBlock()` stereo path (line 159-177) processes L/R K-weighting via SSE2 `Biquad2`, but the power accumulation `mBlockPower += yL * yL + yR * yR` is done in scalar double arithmetic. For blocks of 512+ samples, this inner-loop scalar accumulation is a missed SIMD opportunity.

**Optimization**: Use SSE2 `_mm_mul_pd` and `_mm_add_pd` to compute `yL*yL + yR*yR` and accumulate into a SIMD register, reducing to a scalar sum once at the end of the block (or at each mBlockSize boundary).

**Estimated impact**: The K-weighting biquad is the bottleneck, and it's already SIMD. The power accumulation is 2 multiplies + 1 add per sample — modest. This is a low-priority optimization.

**Alternative**: Accumulate power in a local `__m128d` register (yL*yL in low lane, yR*yR in high lane), then reduce at block boundaries. This avoids the scalar bottleneck of the current `+=` on `double`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LoudnessMeter.cpp` — processBlock() stereo path around line 159-177

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Loudness metering results are bit-identical before and after optimization

## Tests
None (performance optimization, no behavior change)

## Technical Details
- Low priority — the biquad SIMD is the main win; power accumulation is secondary
- Must maintain double precision for power accumulation (float accumulation introduces measurable drift over long blocks)
- Consider loop unrolling by 4 samples for better ILP

## Dependencies
None
