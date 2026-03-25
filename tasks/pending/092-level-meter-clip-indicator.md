# Task 092: Add Clip Indicator to LevelMeter

## Description
Pro-L 2 shows a **small red/orange square block** at the very top of each level meter bar whenever the signal has clipped (reached or exceeded 0 dBFS). This clip indicator stays lit (latched) until clicked to reset. The current `LevelMeter` implementation has no clip indicator — once a signal clips there is no visual record of it.

Reference: `/reference-docs/video-frames/v2-0009.png` (top-right of the level meter area shows a clearly visible red clip box at 0 dB). Also `/reference-docs/video-frames/v1-0040.png`.

## Produces
None

## Consumes
LevelMeterInterface

## Relevant Files
Modify: `M-LIM/src/ui/LevelMeter.h` — add `clipL_`, `clipR_` latch booleans; `setClip(bool, bool)`; `resetClip()`; `mouseDown` override
Modify: `M-LIM/src/ui/LevelMeter.cpp` — draw red clip box in `drawChannel()`, implement latch + reset on click
Read: `M-LIM/src/ui/Colours.h` — use `meterDanger` for clip colour

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds successfully
- [ ] Run: `grep -c "clipL_\|setClip\|resetClip" M-LIM/src/ui/LevelMeter.h` → Expected: at least 3 matches

## Tests
None (UI component)

## Technical Details

**Data:**
```cpp
// In LevelMeter.h
bool clipL_ = false;
bool clipR_ = false;

void setClip(bool left, bool right);   // called from editor timer when peak >= 0 dBFS
void resetClip();                       // called on click
void mouseDown(const juce::MouseEvent&) override;  // detect click on clip box area
```

**Rendering in `drawChannel()`:**
```cpp
// Draw clip indicator at the very top of the bar
const float clipBoxH = 4.0f;
const float clipBoxY = bar.getY();
if (clipped)
{
    g.setColour(MLIMColours::meterDanger);  // bright red
    g.fillRect(bar.getX(), clipBoxY, bar.getWidth(), clipBoxH);
}
else
{
    // Dim indicator when not clipped (show dark block to maintain layout)
    g.setColour(MLIMColours::displayBackground);
    g.fillRect(bar.getX(), clipBoxY, bar.getWidth(), clipBoxH);
}
```

**Click-to-reset:**
Override `mouseDown()` in `LevelMeter`. Check if the click Y coordinate is within the top `clipBoxH` pixels of either bar. If so, call `resetClip()`.

**Integration:**
The `PluginEditor` timer callback should call `levelMeter.setClip(leftPeak >= 0.0f, rightPeak >= 0.0f)` whenever the peak exceeds 0 dBFS (don't reset automatically — user must click). The latch logic:
```cpp
void LevelMeter::setClip(bool left, bool right)
{
    if (left)  clipL_ = true;
    if (right) clipR_ = true;
    repaint();
}
```

## Dependencies
Requires task 020 (LevelMeter component), Requires task 075 (colour fixes)
