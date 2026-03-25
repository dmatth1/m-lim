# Task 161: Fix TransientLimiter Sliding-Window Deque Write Counter Integer Overflow

## Description
The sliding-window maximum deques in `TransientLimiter` use an `int` write counter
(`mMainWriteCount[ch]`, `mSCWriteCount[ch]`). At high oversampling rates this counter
overflows `INT_MAX` (2,147,483,647) within a single session, causing the deque to
malfunction and producing incorrect gain reduction (silence artifacts or missing limiting).

**Location**: `M-LIM/src/dsp/TransientLimiter.h` member declarations;
`M-LIM/src/dsp/TransientLimiter.cpp` lines ~196–220 (deque maintenance).

**Overflow timeline at oversampled rates:**
- 1x (44100 Hz):  overflow at ~13.5 hours  — acceptable
- 2x (88200 Hz):  overflow at ~6.8 hours
- 4x (176400 Hz): overflow at ~3.4 hours
- 8x (352800 Hz): overflow at ~1.7 hours
- 16x (705600 Hz): overflow at ~50 minutes
- 32x (1411200 Hz): **overflow at ~25 minutes** ← clearly unacceptable

**What happens at overflow**: When `mc` wraps from `INT_MAX` to `INT_MIN`, the expression
`mc - lookahead` suffers undefined behavior (signed integer overflow in C++), then the
deque front comparison `md.front().pos < mc - lookahead` may pop all remaining entries,
causing the deque to momentarily report 0 (no peak), resulting in a click or a burst
with no limiting right at the moment of overflow, then stale peaks being stuck in the
deque permanently (no expiry) once `mc` is deeply negative and deque entries are positive.

**Fix**: Change `mMainWriteCount` and `mSCWriteCount` from `std::vector<int>` to
`std::vector<int64_t>`. At 64 bits, overflow at 1411200 Hz takes ~207,000 years.

The `SWDequeEntry::pos` field must also change from `int` to `int64_t` to match.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/TransientLimiter.h` — change counter and pos types to int64_t
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — update any local `int` variables used as counters
Read: `M-LIM/src/dsp/LimiterEngine.cpp` — verify TransientLimiter is called here (context only)

## Acceptance Criteria
- [ ] Run: `grep -n "vector<int>" M-LIM/src/dsp/TransientLimiter.h` → Expected: no match for `mMainWriteCount` or `mSCWriteCount` (they must be `int64_t`)
- [ ] Run: `grep -n "int64_t" M-LIM/src/dsp/TransientLimiter.h` → Expected: at least 3 matches (pos field, mMainWriteCount, mSCWriteCount)
- [ ] Run: `cd build && cmake --build . -j$(nproc) 2>&1 | tail -5` → Expected: no errors, no warnings about signed/unsigned mismatch
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -10` → Expected: all tests pass

## Tests
- Unit: write a test that calls TransientLimiter::process() with a counter pre-seeded to
  INT_MAX - 10 (by processing INT_MAX - 10 silent samples, or by exposing a reset-with-offset
  method) and verifies the deque continues to function correctly after the int32 overflow point
- Unit: confirm at wraparound the peak maximum is still tracked correctly (output does not
  clip or go silent)

## Technical Details

**In TransientLimiter.h**, change the `SWDequeEntry` struct and member vectors:
```cpp
// Before:
struct SWDequeEntry { float value; int pos; };
std::vector<int> mMainWriteCount;
std::vector<int> mSCWriteCount;

// After:
struct SWDequeEntry { float value; int64_t pos; };
std::vector<int64_t> mMainWriteCount;
std::vector<int64_t> mSCWriteCount;
```

**In TransientLimiter.cpp**, update any local `int&` references to the counters:
```cpp
// Before (lines ~196, ~214):
int& mc = mMainWriteCount[ch];
int& sc = mSCWriteCount[ch];

// After:
int64_t& mc = mMainWriteCount[ch];
int64_t& sc = mSCWriteCount[ch];
```

The `SWDeque::push_back(SWDequeEntry e)` and related deque logic will then hold
`int64_t` positions without any other changes needed (all comparisons remain valid).

Also in `prepare()`, the `assign(numChannels, 0)` initializers work for both types.

## Dependencies
None
