# Task 340: Revert Task-333 — Re-hide Input Level Meter

## Description
Task-333 removed `inputMeter_.setVisible(false)` from `PluginEditor.cpp`, making the 30px
input meter strip visible on the left edge of the plugin. This is a CRITICAL regression.

**Measured impact (VisualParityAuditor, 2026-03-26):**
- Left sub-region RMSE (30x378 at x=0,y=30): **26.74%** — up from the ~23.71% baseline before task-333
- The 30px strip shows a stereo meter bar with dB scale labels that does NOT exist in the
  Pro-L 2 reference at that position (the reference waveform goes edge-to-edge)

**What to do:**
In `src/PluginEditor.cpp`, the constructor currently has:
```cpp
addAndMakeVisible (inputMeter_);
```
Add `inputMeter_.setVisible(false);` immediately after that line.

Do NOT touch the `resized()` method — the `bounds.removeFromLeft(kInputMeterW)` call must
stay so the waveform display does not extend into the reserved 30px slot. The slot stays
reserved; the meter is just hidden.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginEditor.cpp` — add `inputMeter_.setVisible(false);` after `addAndMakeVisible(inputMeter_)` in constructor

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: build + screenshot + crop 900x500 → Expected: NO 30px meter strip visible on left edge of waveform; left edge shows waveform gradient directly
- [ ] Run: left sub-region RMSE (30x378 at x=0,y=30) → Expected: lower than current 26.74%

## Tests
None

## Technical Details
In `src/PluginEditor.cpp` constructor body, find:
```cpp
addAndMakeVisible (inputMeter_);
```
Change to:
```cpp
addAndMakeVisible (inputMeter_);
inputMeter_.setVisible (false);
```

The `resized()` method already has:
```cpp
inputMeter_.setBounds (bounds.removeFromLeft (kInputMeterW));
```
This MUST remain unchanged — the 30px slot must stay reserved so the waveform display
has the correct x-offset and width.

## Dependencies
None
