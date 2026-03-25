# Task 174: RotaryKnob — Draw Label Above the Knob Face, Value Below

## Description
In the reference Pro-L 2 (`v1-0035.png`, `prol2-features.jpg`, `prol2-intro.jpg`), all rotary knobs in the control strip show:
- **Label text ABOVE the knob** (e.g., "LOOKAHEAD", "ATTACK", "RELEASE", "TRANSIENTS", "RELEASE")
- **Value text BELOW the knob** (e.g., "0 ms", "0 s", "10 s", "0%", "100%")

The current `RotaryKnob::paint()` draws both label AND value **below** the knob face:
```cpp
// Label text (below knob)
const float textY = knobY + knobSize + 2.0f;
g.drawFittedText(labelText, ..., (int)textY, ...);

// Value text (below label)
g.drawFittedText(cachedValueStr_, ..., (int)textY + 13, ...);
```

This is confirmed by the plugin screenshot (label appears below the knob, not above).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/RotaryKnob.cpp` — reposition label text draw call to above the knob circle
Modify: `M-LIM/src/ui/RotaryKnob.cpp` — adjust `knobY` so the knob sits between label and value
Read: `/reference-docs/video-frames/v1-0035.png` — visual reference showing labels above knobs

## Acceptance Criteria
- [ ] Run: `cmake --build build --config Release -j$(nproc) && echo OK` → Expected: `OK`
- [ ] Run: build plugin, screenshot control strip — expected: knob label (e.g., "LOOKAHEAD") appears **above** the knob, value (e.g., "1.0 ms") appears **below** the knob
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (layout-only change)

## Technical Details

The `RotaryKnob::paint()` function currently allocates the component bounds as:
```
[knobArea] (full height - textHeight)  // knob face + arcs + ticks
[textY = knobY + knobSize + 2]         // label row
[textY + 13]                           // value row
```

Change to:
```
[labelH = 13px]      // label row at top
[knobArea]           // knob in the middle (shrunk by labelH)
[valueH = 13px]      // value row at bottom (already at knobY + knobSize + 2)
```

In `RotaryKnob.cpp::paint()`:
```cpp
// Constants for text heights
const float labelH = 13.0f;
const float valueH = 13.0f;
const float textHeight = labelH + valueH;  // same total reserved space

// Knob starts after label row
const float knobSize = juce::jmin(bounds.getWidth(),
                                   bounds.getHeight() - textHeight);
const float knobX    = bounds.getCentreX() - knobSize * 0.5f;
const float knobY    = bounds.getY() + labelH;   // <-- offset down by label height

// ... rest of knob drawing unchanged ...

// Label ABOVE the knob (at the very top of the component)
g.setColour(MLIMColours::textSecondary);
g.setFont(juce::Font(11.0f));
g.drawFittedText(labelText,
                 juce::Rectangle<int>((int)knobX, (int)bounds.getY(),
                                      (int)knobSize, (int)labelH),
                 juce::Justification::centred, 1);

// Value BELOW the knob (same position as before, just after the knob face)
g.setColour(MLIMColours::textPrimary);
g.setFont(juce::Font(11.0f));
g.drawFittedText(cachedValueStr_,
                 juce::Rectangle<int>((int)knobX, (int)(knobY + knobSize + 2),
                                      (int)knobSize, (int)valueH),
                 juce::Justification::centred, 1);
```

Also update `RotaryKnob::resized()` to shift the slider bounds down by `labelH`:
```cpp
// BEFORE:
slider.setBounds(knobX, bounds.getY(), (int)knobSize, (int)knobSize);

// AFTER:
slider.setBounds(knobX, bounds.getY() + (int)labelH, (int)knobSize, (int)knobSize);
```

## Dependencies
None
