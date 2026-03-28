# Task: ABState — Concurrent Capture/Restore Safety Tests

## Description
`ABState` is accessed from both the message thread (UI button clicks for A/B toggle)
and indirectly from `getStateInformation()`/`setStateInformation()` (which can be called
by the host from a non-message thread in some DAWs). The current implementation has no
mutex or atomic guards:

```cpp
// ABState.cpp
void ABState::captureState(juce::AudioProcessorValueTreeState& apvts)
{
    (mCurrentSlot == 0 ? mStateA : mStateB) = apvts.copyState();
    ...
}
```

Concurrent `captureState()` + `restoreState()` calls targeting the same slot can produce
a torn write/read of `juce::ValueTree` objects (which are reference-counted, not atomic).
This is a real-world risk during A/B switch when the host simultaneously saves state.

The existing `test_ab_state.cpp` has no concurrent tests whatsoever.

Add the following tests:

1. **Concurrent capture + restore on slot A**: two threads race — one calls `captureState()`
   repeatedly (50×), the other calls `restoreState()` repeatedly (50×). Require: no crash,
   no sanitiser report (TSAN/ASAN), and APVTS state is a valid ValueTree after the race
   (not a zero-length or corrupt object).

2. **Concurrent toggle + capture**: one thread alternates `toggle()` (50×), another calls
   `captureState()` (50×). Require: no crash; `mCurrentSlot` always ends in {0,1} (never
   out of range).

3. **Restore with no prior capture (empty slot)**: call `restoreState()` immediately after
   construction without calling `captureState()`. Require: no crash, APVTS state unchanged
   (verify by reading a known parameter before and after).

4. **CopyAtoB with empty A slot**: call `copyAtoB()` when slot A has never been populated.
   Require: no crash, slot B remains empty (not set to a default/zero state that would
   overwrite valid B content).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/tests/state/test_ab_state.cpp` — add new TEST_CASE blocks
Read: `M-LIM/src/state/ABState.h` — understand public interface
Read: `M-LIM/src/state/ABState.cpp` — understand implementation (no mutex)
Read: `M-LIM/tests/state/state_test_helpers.h` — reuse APVTS fixture if one exists

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "ABState" --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -c "thread\|concurrent\|race\|empty.*slot\|slot.*empty" M-LIM/tests/state/test_ab_state.cpp -i` → Expected: >= 6

## Tests
- Unit: `tests/state/test_ab_state.cpp::test_restore_before_capture_no_crash` — restore with empty slot is a no-op
- Unit: `tests/state/test_ab_state.cpp::test_copyatob_empty_slot_no_crash` — copyAtoB with uninitialized A
- Integration: `tests/state/test_ab_state.cpp::test_concurrent_capture_restore_no_crash` — race between capture and restore
- Integration: `tests/state/test_ab_state.cpp::test_concurrent_toggle_capture_no_crash` — race between toggle and capture

## Technical Details
- Use `std::thread` and `std::atomic<bool>` start flag to synchronize threads.
- For the "APVTS state is valid" assertion: after the race, call `apvts.state.getType()` and require it equals the expected ValueTree type string (not an empty Identifier).
- These tests expose thread-safety bugs rather than verifying thread-safe guarantees; if they find a real data race, the worker should file a note in the task output and create a follow-up bug task rather than adding internal locking (that decision belongs to the maintainer).
- The "no prior capture" and "copyAtoB empty slot" tests are single-threaded and straightforward — add them regardless of whether concurrency tests are run.

## Dependencies
None
