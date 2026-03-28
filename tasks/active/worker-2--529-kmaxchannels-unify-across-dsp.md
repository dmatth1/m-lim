# Task 529: Unify kMaxChannels Across DSP Classes

## Description
Channel capacity limits are inconsistent:
- `SidechainFilter.h`: `kMaxChannels = 2`
- `TransientLimiter.h`: `kMaxChannels = 8`
- `LimiterEngine.h`: `kMaxChannels = 2`

TransientLimiter allocates for 8 channels but will never receive more than 2 from LimiterEngine. This wastes memory and creates confusion about actual channel support.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/TransientLimiter.h` — change kMaxChannels to 2
Read: `M-LIM/src/dsp/SidechainFilter.h` — verify kMaxChannels = 2
Read: `M-LIM/src/dsp/LimiterEngine.h` — verify kMaxChannels = 2

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Run: `grep -rn "kMaxChannels" M-LIM/src/dsp/` → Expected: all show value 2

## Tests
None (constant change, existing tests validate behavior)

## Technical Details
Either:
1. Change TransientLimiter::kMaxChannels to 2 (matching the rest), or
2. Define a single `DspConstants::kMaxChannels = 2` in `DspUtil.h` and reference it everywhere.

Option 2 is preferable as a single source of truth.

## Dependencies
None
