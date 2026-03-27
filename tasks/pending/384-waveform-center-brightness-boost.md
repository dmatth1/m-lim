# Task: Waveform Center Brightness Boost — Tent-Shaped Overlay + Idle Fill Increase

## Description

Color sampling of the current M-LIM vs reference reveals a systematic brightness gap
in the waveform display mid-zone:

| Zone       | M-LIM current | Reference | Gap     |
|------------|--------------|-----------|---------|
| Upper 30%  | #2E2E3C      | #302D33   | ~3 units ✓ (matches well) |
| Mid 30–60% | #474F6C      | #6D7694   | +39 units (biggest gap) |
| Lower 60%  | #5A6891      | #8086A1   | +28 units |

The mid-zone (30–60% of height) is 39 units too dark. This is the center of the waveform
display where the reference shows the densest waveform content. The existing mid-zone
boost (task 376) fades from 0.42f at 28% to 0.0f at 60%, giving only ~0.22f effective
alpha at the image center (50% height = 69% into the gradient).

**Fix — two changes to `WaveformDisplay::drawBackground()`:**

### Change 1: Add center-peaked tent fill

After the existing mid-zone boost block (the block starting with `const float midTop =`),
add a NEW "center tent" fill that peaks at 50% height:

```cpp
// Center tent brightness boost — 32%→50% rising, 50%→68% falling
// Addresses the 39-unit mid-zone gap (ref #6D7694 vs current #474F6C).
{
    const float cTop = area.getY() + area.getHeight() * 0.32f;
    const float cMid = area.getY() + area.getHeight() * 0.50f;
    const float cBot = area.getY() + area.getHeight() * 0.68f;
    juce::Colour cCol { 0xff828AA5 };  // same steel-blue as midFillColour above

    // Rising half: 0.0 at cTop → 0.55 at cMid
    juce::ColourGradient upGrad (
        cCol.withAlpha (0.0f),   0.0f, cTop,
        cCol.withAlpha (0.55f),  0.0f, cMid,
        false);
    g.setGradientFill (upGrad);
    g.fillRect (area.getX(), cTop, area.getWidth(), cMid - cTop);

    // Falling half: 0.55 at cMid → 0.0 at cBot
    juce::ColourGradient downGrad (
        cCol.withAlpha (0.55f),  0.0f, cMid,
        cCol.withAlpha (0.0f),   0.0f, cBot,
        false);
    g.setGradientFill (downGrad);
    g.fillRect (area.getX(), cMid, area.getWidth(), cBot - cMid);
}
```

### Change 2: Increase idle fill opacity (bottom)

In the existing idle fill block, change bottom alpha from `0.80f` to `0.88f`:

```cpp
// BEFORE:
MLIMColours::inputWaveform.withAlpha (0.80f),  0.0f, area.getBottom(),
// AFTER:
MLIMColours::inputWaveform.withAlpha (0.88f),  0.0f, area.getBottom(),
```

**Expected composite at center (50%):**
- Center tent: rgb(130,138,165) × 0.55 + base × 0.45 → +36 units brightness
- Result: ~rgb(105,113,143) vs target rgb(109,118,148) — within 5 units

**Expected improvement:** wave region RMSE ~0.5–0.9pp improvement.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/WaveformDisplay.cpp` — `drawBackground()` method
Read: `src/ui/Colours.h` — `inputWaveform`, `displayGradientTop/Bottom` constants

## Acceptance Criteria
- [ ] Run: build + screenshot + wave region RMSE → Expected: ≤ 18.5% (improvement from 18.96%)
- [ ] Run: full image RMSE → Expected: ≤ 20.74% (no regression from wave 18 baseline)
- [ ] Run: visual check → Expected: waveform center appears slightly brighter, no harsh banding; upper zone (0–30%) unchanged; no visible flat-fill rectangles

## Tests
None

## Technical Details

In `WaveformDisplay::drawBackground()`, the code blocks appear in this order:
1. Background gradient fill (~line 283–290)
2. Idle fill (bottom 56%, alpha 0.0→0.80) (~line 296–308) ← Change 2: 0.80f → 0.88f
3. Mid-zone boost (28%–60%, alpha 0.42→0.0) (~line 309–325) ← Insert Change 1 AFTER this block
4. Grid lines and dB overlay labels

Insert Change 1 immediately after the closing `}` of block 3.

Build command:
```bash
export CCACHE_DIR=/build-cache
cmake --build /workspace/M-LIM/build --target MLIM_Standalone -j$(nproc)
```

Measure:
```bash
convert /reference-docs/reference-screenshots/prol2-main-ui.jpg \
    -crop 1712x1073+97+32 +repage -resize 900x500! /tmp/ref.png

pkill -f "Standalone/M-LIM" 2>/dev/null; sleep 1
DISPLAY=:99 /workspace/M-LIM/build/MLIM_artefacts/Release/Standalone/M-LIM &>/dev/null &
sleep 8; DISPLAY=:99 scrot /tmp/raw.png; pkill -f "Standalone/M-LIM"
convert /tmp/raw.png -crop 908x500+509+325 +repage -resize 900x500! /tmp/mlim.png

# Wave RMSE
convert /tmp/mlim.png -crop 640x500+0+0 +repage /tmp/wave-mlim.png
convert /tmp/ref.png  -crop 640x500+0+0 +repage /tmp/wave-ref.png
compare -metric RMSE /tmp/wave-ref.png /tmp/wave-mlim.png /dev/null 2>&1

# Full RMSE
compare -metric RMSE /tmp/ref.png /tmp/mlim.png /dev/null 2>&1
```

If wave RMSE worsens (> 18.96%), try reducing center tent alpha from 0.55f to 0.45f.
If visible banding, reduce cTop from 0.32f to 0.35f.

## Dependencies
None
