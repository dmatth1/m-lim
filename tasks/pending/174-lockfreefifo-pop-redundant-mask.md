# Task 174: Fix Inconsistent Masking Style in LockFreeFIFO

## Description
`LockFreeFIFO::pop()` in `MeterData.h` (lines 84–93) applies `& mMask` to `mWritePos` during the empty check but not to `read` (which is `mReadPos`):

```cpp
const int read = mReadPos.load(std::memory_order_relaxed);
if (read == (mWritePos.load(std::memory_order_acquire) & mMask))
    return false;  // empty
```

Meanwhile `isFull()` (lines 57–62) applies `& mMask` to both `write+1` and `read`:
```cpp
return ((write + 1) & mMask) == (read & mMask);
```

And `push()` (lines 68–78) applies `& mMask` to `mReadPos`:
```cpp
if (next == (mReadPos.load(std::memory_order_acquire) & mMask))
```

The positions are always stored pre-masked (see `push()` line 76: `mWritePos.store(next, ...)` where `next = (write+1) & mMask`, and `pop()` line 91: `mReadPos.store((read + 1) & mMask, ...)`). The redundant masks are therefore harmless, but the **inconsistent style** across the four methods makes it hard to reason about correctness at a glance and suggests the code was written without a clear invariant in mind.

Also: `pop()` line 90 reads `mBuffer[static_cast<std::size_t>(read)]` without the mask, which is correct only because `read` is pre-masked — but this relies on a non-obvious invariant that should be stated in a comment.

Clean up:
1. Remove the redundant `& mMask` on already-masked values in `isFull()`, `push()`, and `pop()`.
2. Add a one-line comment in `pop()` and `push()` noting that positions are always stored pre-masked.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/MeterData.h` — clean up mask usage and add invariant comments in LockFreeFIFO

## Acceptance Criteria
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure -R test_lockfree_fifo 2>&1 | tail -10` → Expected: all tests pass

## Tests
None

## Technical Details
The invariant to document: both `mWritePos` and `mReadPos` are **always stored as values already masked to [0, mMask]**. The `& mMask` applied to them in comparisons is therefore a no-op and should be removed for clarity.

After cleanup the empty check in `pop()` becomes:
```cpp
if (read == mWritePos.load(std::memory_order_acquire))
    return false;
```

And the full check in `push()` becomes:
```cpp
if (next == mReadPos.load(std::memory_order_acquire))
    return false;
```

Add above the `push()` definition:
```cpp
// Invariant: mWritePos and mReadPos are always stored pre-masked to [0, mMask].
```

## Dependencies
None
