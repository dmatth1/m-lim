# Task 319: Loudness Histogram Scale — Don't Skip Labels Near Target

## Description
The loudness histogram scale in `LoudnessPanel::drawHistogram()` skips dB labels that are
within 3 LUFS of the target to avoid overlap with the target indicator line. However, this
creates a jarring 10 LUFS gap in the scale.

With the default target of −14 LUFS and 5-LUFS label intervals:
- −10 label shows (|−10 − (−14)| = 4.0 > 3.0)
- −15 label is **skipped** (|−15 − (−14)| = 1.0 < 3.0)
- −20 label shows

This creates a gap from −10 to −20 with only the yellow −14 target line in between.
The reference shows labels consistently without large gaps.

Fix: Reduce skip threshold from 3.0 LUFS to 1.5 LUFS so only labels that would truly
overlap (within 1.5 LUFS = about 4px at current scale) are hidden.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LoudnessPanel.cpp` — `drawHistogram()`, the label skip condition at lines 374–376

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc)` → Expected: build succeeds
- [ ] Run: launch standalone, observe right panel histogram scale with default −14 Strm target → Expected: scale shows −5, −10, −15, −20, −25, −30, −35 (no 10-LUFS gap), with target marker at −14 between −15 and −13 area

## Tests
None

## Technical Details
In `drawHistogram()`, find this block:
```cpp
// Skip if too close to the target label
if (std::abs(static_cast<float>(dB) - targetLUFS_) < 3.0f)
    continue;
```
Change the threshold from `3.0f` to `1.5f`:
```cpp
if (std::abs(static_cast<float>(dB) - targetLUFS_) < 1.5f)
    continue;
```

This allows −15 to show (1.0 LUFS from −14 > 1.5? No — 1.0 < 1.5, still skipped).

Actually, use a tighter threshold to show the label. The visual distance between −14 and −15 is:
  (1.0 / 35.0) × barHeight ≈ 1/35 × (totalHeight − 2×kBarPad)
At typical panel height ~200px visible bar area: 1/35 × 194 ≈ 5.5px per LUFS.
So −15 is only 5.5px from the target line. Change threshold to `0.75f` to only skip labels
that would literally overlap (within 0.75 LUFS ≈ 4px of target line):
```cpp
if (std::abs(static_cast<float>(dB) - targetLUFS_) < 0.75f)
    continue;
```

This ensures −15 label shows up (distance 1.0 > 0.75) and only labels at, say, −14.5 or −13.5
would be skipped.

## Dependencies
None
