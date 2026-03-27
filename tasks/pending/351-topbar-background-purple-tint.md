# Task 351: TopBar Background — Add Subtle Purple Tint

## Description

The TopBar background gradient is neutral dark gray. The reference Pro-L 2 header has a
slightly purple-tinted dark background. A minor color tweak will better match the reference
and reduce the TopBar's contribution to full-image RMSE.

**Measured pixel values at crop y=10, x=300–800 (auditor, 2026-03-27):**

| Source       | Background color | R  | G  | B  |
|------------- |------------------|----|----|----|
| M-LIM        | gradient ≈ mid   | 42 | 42 | 43 |
| Reference    | #29252B          | 41 | 37 | 43 |

The reference has **G ≈ 37** (vs M-LIM G=42) — a 5-unit green deficit creating the purple tint.
R and B are nearly identical.

**Current TopBar.cpp paint():**
```cpp
juce::ColourGradient bg (juce::Colour (0xff252525), 0.0f, 0.0f,
                         juce::Colour (0xff1F1F1F), 0.0f, bounds.getHeight(), false);
```

Both colours are neutral gray (R=G=B). Change them to have a slight purple tint:
- Top: `0xff252228`  (was 0xff252525; -3 G, +3 B)
- Bottom: `0xff1F1C22` (was 0xff1F1F1F; -3 G, +3 B)

This reduces the green channel by ~3 and increases blue by ~3, matching the reference's
slight purple undertone without making the change visible to the naked eye.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/TopBar.cpp` — change gradient colours in `TopBar::paint()`

## Acceptance Criteria
- [ ] Run: `grep -A3 'ColourGradient bg' M-LIM/src/ui/TopBar.cpp` → Expected: top color NOT `0xff252525`
- [ ] Run: build succeeds → `cmake --build M-LIM/build -j$(nproc) 2>&1 | tail -1` → Expected: `Built target`
- [ ] Run: launch on Xvfb, crop 1x1 at approximately the TopBar center (x=400, y=10 of the 900x500 crop), verify pixel has slight purple tint (B > G by at least 2 counts)

## Tests
None

## Technical Details

**TopBar.cpp change in `TopBar::paint()`:**
```cpp
// Before:
juce::ColourGradient bg (juce::Colour (0xff252525), 0.0f, 0.0f,
                         juce::Colour (0xff1F1F1F), 0.0f, bounds.getHeight(), false);

// After:
juce::ColourGradient bg (juce::Colour (0xff252228), 0.0f, 0.0f,  // slight purple tint
                         juce::Colour (0xff1F1C22), 0.0f, bounds.getHeight(), false);
```

Both colors keep the same overall luminance (~#232 range) but shift the hue from neutral gray
toward the dark purple-navy of the Pro-L 2 header background (#29252B target).

## Dependencies
None
