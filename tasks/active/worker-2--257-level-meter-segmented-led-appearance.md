# Task 257: Level Meter — Segmented LED-Strip Appearance

## Description
The reference Pro-L 2 level meters use a **segmented LED-strip style** where each color zone is
divided into small rectangular segments (~3 px tall) with ~1 px gaps between them, giving the
characteristic VU-meter look. M-LIM currently fills each color zone with a single solid rectangle,
which looks flat and does not match the reference.

Reference evidence: `prol2-metering.jpg` clearly shows small horizontal segments with gaps in all
three color zones (blue safe zone, red danger zone).

**Implementation approach** (in `LevelMeter::drawChannel`):
1. After computing `fillTop` and the zone boundaries, instead of calling `g.fillRect()` once per
   zone, iterate over the zone height in steps of `kSegH + kSegGap` px, drawing one `kSegH`-tall
   filled rectangle per step.
2. Each segment should be clipped to its zone boundaries so segments never cross color-zone lines.
3. Keep the peak-hold line and clip indicator as-is (they are drawn outside the segment loop).

Suggested constants (add near the top of `LevelMeter.cpp`):
```cpp
constexpr float kSegH   = 3.0f;  // segment height in pixels
constexpr float kSegGap = 1.0f;  // gap between segments
```

Do NOT use segments for the peak-hold line or the clip indicator at the top — those remain solid.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/LevelMeter.cpp` — `drawChannel()` method: replace solid zone `fillRect` calls with
  segment loop
Read:   `src/ui/LevelMeter.h` — constants `kWarnDB`, `kDangerDB`, `kMinDB`, `kMaxDB`
Read:   `/reference-docs/reference-screenshots/prol2-metering.jpg` — reference showing segmented bars
Read:   `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — main UI reference

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -5` →
      Expected: build succeeds with no errors
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` →
      Expected: all tests pass
- [ ] Visual check: launch standalone on Xvfb, take screenshot, compare level meter bars — they
      should show clearly visible horizontal segment gaps (LED-strip look) matching `prol2-metering.jpg`

## Tests
None (visual rendering change — no unit tests required)

## Technical Details
The zone fill loop in `drawChannel()` currently does:
```cpp
g.setColour (MLIMColours::meterDanger);
g.fillRect (bar.withTop (top).withBottom (bot));
```
Replace each such call with a segment loop:
```cpp
g.setColour (MLIMColours::meterDanger);
for (float sy = top; sy < bot; sy += kSegH + kSegGap)
{
    float segBottom = juce::jmin (sy + kSegH, bot);
    if (segBottom > sy)
        g.fillRect (bar.withTop (sy).withBottom (segBottom));
}
```
Apply the same loop to all three zone fills (danger, warning, safe).

The peak-hold line (`g.fillRect (bar.getX(), peakY, bar.getWidth(), kPeakLineH)`) remains as a solid
line — do NOT segment it.

## Dependencies
None
