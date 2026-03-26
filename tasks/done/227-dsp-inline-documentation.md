# Task 227: DSP Inline Code Documentation

## Description
Add inline documentation (Doxygen-style `/** */` block comments) to the complex DSP algorithms. The goal is to make the code understandable to anyone with audio DSP knowledge reading the source for the first time.

Document the following:

### TransientLimiter (Stage 1)
- Class-level doc: role in the dual-stage chain, lookahead design
- `process()`: explain the lookahead peak scan, gain computation, attack/hold/release state machine
- `computeGain()`: explain the knee, threshold comparison, gain reduction formula

### LevelingLimiter (Stage 2)
- Class-level doc: role as slow-release leveling stage, interaction with Stage 1 output
- `process()`: explain the release envelope follower, how it differs from Stage 1

### TruePeakDetector
- Class-level doc: ITU-R BS.1770-4 inter-sample peak detection via 4x oversampling + FIR
- `process()`: explain the FIR filter coefficients source and the peak measurement method
- Note the standard reference (ITU-R BS.1770-4)

### LoudnessMeter
- Class-level doc: ITU-R BS.1770-4 K-weighted gated loudness measurement
- `process()`: explain the two-stage K-weighting filter chain (pre-filter + RLB weighting)
- `getIntegratedLUFS()`: explain the absolute and relative gating algorithm
- Note the standard reference (EBU R128 / ITU-R BS.1770-4)

### LimiterEngine
- Class-level doc: orchestration of the full DSP chain
- `process()` or equivalent: explain the signal flow (input gain → oversample → Stage1 → Stage2 → ceiling → DC → dither)

### Dither
- `process()`: explain TPDF dither generation and the noise shaping filter

Do NOT add comments to trivial getter/setter methods or obvious code. Only document non-obvious algorithmic logic.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/TransientLimiter.h` — add class doc
Modify: `src/dsp/TransientLimiter.cpp` — add method docs
Modify: `src/dsp/LevelingLimiter.h` — add class doc
Modify: `src/dsp/LevelingLimiter.cpp` — add method docs
Modify: `src/dsp/TruePeakDetector.h` — add class doc
Modify: `src/dsp/TruePeakDetector.cpp` — add method docs
Modify: `src/dsp/LoudnessMeter.h` — add class doc
Modify: `src/dsp/LoudnessMeter.cpp` — add method docs
Modify: `src/dsp/LimiterEngine.h` — add class doc
Modify: `src/dsp/Dither.h` — add class doc

## Acceptance Criteria
- [ ] Run: `grep -l "/\*\*" /workspace/M-LIM/src/dsp/TransientLimiter.h /workspace/M-LIM/src/dsp/LoudnessMeter.h /workspace/M-LIM/src/dsp/TruePeakDetector.h` → Expected: all 3 files listed (Doxygen comments present)
- [ ] Run: `grep -c "/\*\*" /workspace/M-LIM/src/dsp/LoudnessMeter.cpp` → Expected: at least 3 (one per major method)
- [ ] Run: `cd /workspace/M-LIM/build && cmake --build . -j$(nproc) 2>&1 | tail -5` → Expected: build still succeeds after adding comments

## Tests
None

## Technical Details
- Use `/** ... */` for Doxygen-compatible block comments
- Use `@brief`, `@details`, `@note` tags where helpful
- Reference standards: ITU-R BS.1770-4, EBU R128, AES17
- Keep comments accurate to the actual implementation — read the code before writing comments

## Dependencies
Requires task 224
