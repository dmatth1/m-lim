# Task 266: Control Strip Gradient Background Too Bright

## Description
The control strip gradient background is approximately 2.5× too bright compared to the
reference Pro-L 2. This is the most visually jarring colour mismatch in the idle plugin
screenshot and is the primary contributor to the 22.2% control strip RMSE.

**Measured reference values (from /reference-docs/video-frames/v1-0020.png):**
- Top of control strip (just below waveform divider): `#3A3641` (RGB 58, 54, 65)
- Middle/lower area: `#302D37` (RGB 48, 45, 55)
- Bottom: `#2A2734` (RGB 42, 39, 52)
- Cross-checked against prol2-intro.jpg: `#42404B` to `#45444A`

**Current M-LIM values (too bright/blue):**
- `controlStripTop = 0xff75809A` → renders as ~`#737B90` (RGB 115, 123, 144)
- `controlStripBottom = 0xff3E4255` → renders as ~`#3C4053`
- The top is about 2× too bright; the strip has too much blue saturation.

**Fix — update two constants in `src/ui/Colours.h`:**

```cpp
// Change from:
const juce::Colour controlStripTop    { 0xff75809A };
const juce::Colour controlStripBottom { 0xff3E4255 };

// Change to:
const juce::Colour controlStripTop    { 0xff3B3840 };  // dark purple-gray, matches ref ~#3A3641
const juce::Colour controlStripBottom { 0xff282530 };  // darker at bottom, matches ref ~#2A2734
```

Also update the comment on these lines (currently says "matches reference #7D87A2" which
is no longer accurate).

**Secondary: verify `algoButtonInactive` still has sufficient contrast.**
`algoButtonInactive = 0xff303848` (RGB 48, 56, 72). Against the new darker
`controlStripTop = 0xff3B3840` (RGB 59, 56, 64), the button will appear as a slightly
blue-tinted element on a near-neutral strip — acceptable contrast. No change needed, but
verify visually.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/Colours.h` — update `controlStripTop`, `controlStripBottom`; update
  their comment
Read:   `src/ui/ControlStrip.cpp` — verify gradient usage in `paint()`
Read:   `/reference-docs/video-frames/v1-0020.png` — reference colour for control strip
Read:   `/reference-docs/reference-screenshots/prol2-intro.jpg` — secondary reference

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -3` →
      Expected: build succeeds with no errors
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` →
      Expected: all tests pass
- [ ] Visual check: launch standalone on Xvfb, take screenshot — the knob/control area
      should be a dark near-black purple-gray (matching `v1-0020.png` control strip), NOT
      the previous medium blue-gray
- [ ] Pixel sample check: sample pixel in gap between knobs (approx x=350, y=420 in
      900×500 crop). Expected: approximately RGB(40–65, 38–62, 48–75) range, NOT
      the previous `#606980`

## Tests
None (colour styling change — no unit tests required)

## Technical Details
Only `Colours.h` needs to change. The gradient is applied in `ControlStrip::paint()`:

```cpp
juce::ColourGradient bg (MLIMColours::controlStripTop,  0.0f, 0.0f,
                         MLIMColours::controlStripBottom, 0.0f, h, false);
g.setGradientFill (bg);
g.fillRect (bounds);
```

The `panelOverlay = 0x20FFFFFF` used for the Channel Linking section overlay (adds 12.5%
white) will composite against the new darker background to give approximately `#5A5661` —
slightly lighter than the strip, which is the correct raised-panel appearance.

The ADVANCED button uses `displayBackground = 0xff111118` for its fill (handled in
LookAndFeel), which is already near-black and will look correct against the darker strip.

## Dependencies
None
