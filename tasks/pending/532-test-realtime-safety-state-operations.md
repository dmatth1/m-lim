# Task 532: Realtime Safety Tests for State Operations During Audio

## Description
`test_realtime_safety.cpp` tests that processBlock makes no heap allocations during steady-state DSP. However, it doesn't test what happens when state operations (preset load, A/B toggle, undo/redo) occur while processBlock is running. These operations call `apvts.replaceState()` on the message thread, which could trigger allocations or locks visible to the audio thread. A test should verify that processBlock remains allocation-free even when state operations happen concurrently.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/PluginProcessor.cpp` — processBlock, setStateInformation, parameterChanged
Read: `tests/integration/test_realtime_safety.cpp` — existing allocation tracking
Modify: `tests/integration/test_realtime_safety.cpp` — add state-operation tests

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "RealtimeSafety" --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Integration: `tests/integration/test_realtime_safety.cpp::test_no_alloc_during_state_restore` — run processBlock in a loop on one thread while calling setStateInformation on another; verify audio thread allocation count stays at 0
- Integration: `tests/integration/test_realtime_safety.cpp::test_no_alloc_during_parameter_sweep` — sweep all parameters rapidly via setValueNotifyingHost while processBlock runs; verify no audio-thread allocations

## Technical Details
Use the existing allocation tracking mechanism. Spawn a background thread for state operations. Run 100+ processBlock calls while state changes happen. Count allocations only on the audio thread (the one calling processBlock). Note: JUCE's APVTS parameter changes via atomics should be allocation-free, but replaceState() is a different path.

## Dependencies
None
