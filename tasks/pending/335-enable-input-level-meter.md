# Task 335: Re-enable Input Level Meter Strip

## Description
The input level meter (LevelMeter component, kInputMeterW=30) is currently hidden:
```cpp
inputMeter_.setVisible (false);  // in PluginEditor::resized()
```

In the reference (prol2-main-ui.jpg at 900x500), the left edge of the waveform area (x=0-25)
shows DARK colors at rest: #262127 to #312128 at y=50-100. This matches what a level meter
strip would show (dark barTrackBackground with scale labels).

Our current M-LIM shows the waveform gradient in this area (#676369 at y=50), which is
significantly lighter than the reference (#262127). This contributes to the 23.71% RMSE in
the left meters region (30x378+0+30).

Pixel measurements at x=25, y=100 (reference vs M-LIM):
  Reference: #1D1B20 (very dark)
  M-LIM:     #626370 (medium gray — waveform gradient)
  Difference: ΔR=69, ΔG=68, ΔB=80 — very large!

By re-enabling the input meter (30px wide), the left 30px of the comparison region would show
the dark meter background, much closer to the reference.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.cpp` — re-enable inputMeter_, set correct bounds in resized()
Read: `M-LIM/src/PluginEditor.h` — check kInputMeterW constant (30px)
Read: `M-LIM/src/ui/LevelMeter.cpp` — verify meter renders correctly at rest (dark background)

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "screenshots/task-335-after.png" && stop_app` → Expected: screenshot saved
- [ ] Visual: A narrow dark vertical strip (input level meter) appears on the far left of the waveform area, matching the reference's dark left edge

## Tests
None

## Technical Details
In `M-LIM/src/PluginEditor.cpp`, in `resized()`:

```cpp
// BEFORE:
// Input level meter hidden — waveform extends to left edge (no left strip)
inputMeter_.setVisible (false);

// AFTER:
// Input level meter: narrow strip on the left edge of waveform area
inputMeter_.setVisible (true);
inputMeter_.setBounds (bounds.removeFromLeft (kInputMeterW));
```

The `kInputMeterW = 30` constant already exists in PluginEditor.h. The `removeFromLeft` call
should come BEFORE the right-side components are removed, so the waveform gets the remaining
center area after left meter, GR meter, output meter, and loudness panel are allocated.

Note: The input gain badge (inputGainSlider_) position might need adjustment since the
waveform x-position changes. Update `badgeX` accordingly:
```cpp
const int badgeX = waveformDisplay_.getX() + 4;  // (unchanged — uses waveformDisplay_ bounds)
```
This should be fine since it references waveformDisplay_.getX() dynamically.

Also verify the input meter layout in LevelMeter.cpp properly draws the scale on the right
side (kScaleWidth = 20 within the component). At 30px wide:
  - Bars: (30-20) * 0.46 ≈ 4.6px each (very narrow but visible)
  - Gap: (30-20) * 0.08 ≈ 0.8px
  - Scale: 20px on right side

If bars are too narrow at 30px width, consider reducing kScaleWidth to 14px within LevelMeter
(making bars 7.4px each, more visible). This is optional — the primary goal is matching the
dark reference background, not bar width accuracy.

## Dependencies
Requires tasks 328 and 329 (layout reverts) to be complete so the overall layout is stable
before adding the input meter back.
