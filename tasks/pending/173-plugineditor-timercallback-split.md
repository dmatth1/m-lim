# Task 173: Split PluginEditor::timerCallback() into Private Helper Methods

## Description
`MLIMAudioProcessorEditor::timerCallback()` (`PluginEditor.cpp` lines 186–235) does seven distinct things in 50 lines:

1. Drains the meter FIFO (while loop)
2. Converts raw linear peak values to dBFS
3. Updates level meter components
4. Updates peak hold state for input and output
5. Updates gain reduction meter
6. Updates clip indicators
7. Updates all four loudness panel readouts

This violates the single-responsibility principle and makes the function hard to read and test. The aging of peak hold counters (lines 225–234) is also mixed outside the FIFO drain loop, creating temporal coupling that is easy to break.

Extract the per-frame metering update into a private method `applyMeterData(const MeterData&)` and the counter aging into `agePeakHoldCounters()`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.cpp` — refactor timerCallback and add private helpers
Modify: `M-LIM/src/PluginEditor.h` — declare new private methods

## Acceptance Criteria
- [ ] Run: `awk '/timerCallback/{found=1; count=0} found{count++} /^}$/{if(found && count>1){print count; found=0}}' M-LIM/src/PluginEditor.cpp` → Expected: prints a number ≤ 15 (timerCallback body is now a short orchestrator)
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0

## Tests
None

## Technical Details
Suggested private method signatures to add to `PluginEditor.h`:
```cpp
void applyMeterData (const MeterData& data);   // called once per FIFO item
void agePeakHoldCounters() noexcept;           // called once per timer tick
```

`timerCallback()` becomes:
```cpp
void MLIMAudioProcessorEditor::timerCallback()
{
    MeterData data;
    while (audioProcessor.getMeterFIFO().pop (data))
        applyMeterData (data);
    agePeakHoldCounters();
}
```

Move the existing peak-hold lambda inside `agePeakHoldCounters()`. The overall logic must be unchanged — this is a structural refactor only.

## Dependencies
None
