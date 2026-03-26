# Task 300: Algorithm Selector — Reduce to Single Knob Slot for Larger Knobs

## Description
The AlgorithmSelector currently occupies **2 knob slots** (2/7 of the control strip width, about
234px). In the Pro-L 2 reference, the algorithm/style label appears as a compact pill button below
the "STYLE" label — approximately 60-80px wide. The oversized `< TRANSPARENT >` navigation box
with large arrows takes excessive space and looks visually different from the reference.

Additionally, with the algorithm selector at 2 slots, each knob slot is 1/7 of available width
(~117px). If the selector is reduced to 1 slot, knob slots increase to 1/6 of width (~137px),
making knobs ~17% wider — closer to the reference's proportionally larger knobs.

**Fix**:
1. In `ControlStrip::resized()`, change algorithm selector from `knobW * 2` to `knobW * 1`:
   - Change `int knobW = knobRow.getWidth() / 7` → `int knobW = knobRow.getWidth() / 6`
   - `algorithmSelector_.setBounds(knobRow.removeFromLeft(knobW))` (one slot only)
2. Resize the `prevButton_` and `nextButton_` in `AlgorithmSelector::resized()` from 20px to 14px
   to fit the narrower slot.
3. Use a smaller font (`kFontSizeSmall = 9.0f`) for the name label inside the selector.

After this change the 6-slot distribution is: algo(1) + lookahead(1) + attack(1) + release(1) +
CL-transients(1) + CL-release(1) = 6 slots total.

Note: Task 293 (ADVANCED back to ControlStrip) adds a narrow 12px slot between CL-release and
OUTPUT. Coordinate so the knobW calculation accounts for that 12px.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/ControlStrip.cpp` — change knob slot count from 7 to 6 in `resized()`
Modify: `M-LIM/src/ui/AlgorithmSelector.cpp` — reduce `prevButton_`/`nextButton_` from 20px to 14px
        in `resized()`; reduce font from kFontSizeMedium to kFontSizeSmall in constructor
Read: `/workspace/screenshots/ref-style-area.png` — reference shows compact "Modern" style button

## Acceptance Criteria
- [ ] Run: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone` → Expected: exits 0
- [ ] Visual: Algorithm selector fits in one knob slot — no wider than ~140px (1/6 of strip)
- [ ] Visual: Knobs (LOOKAHEAD, ATTACK, RELEASE etc.) appear ~17% larger than before
- [ ] Visual: Algorithm name text remains legible within the smaller selector area
- [ ] Visual: Navigation arrows (‹ ›) are still clickable at their reduced 14px width

## Tests
None

## Technical Details
In `ControlStrip::resized()`:
```cpp
// Change from 7-slot to 6-slot distribution (algo now takes 1 slot, not 2)
// Also reserve 12px for ADVANCED tab (if task 293 is done)
int knobW = (knobRow.getWidth() - 12) / 6;  // was / 7

algorithmSelector_.setBounds (knobRow.removeFromLeft (knobW));    // was knobW * 2
lookaheadKnob_.setBounds     (knobRow.removeFromLeft (knobW));
attackKnob_.setBounds        (knobRow.removeFromLeft (knobW));
releaseKnob_.setBounds       (knobRow.removeFromLeft (knobW));
channelLinkTransientsKnob_.setBounds (knobRow.removeFromLeft (knobW));
auto advancedSlot = knobRow.removeFromLeft (12);   // ADVANCED tab
advancedButton_.setBounds (advancedSlot);          // (from task 293)
channelLinkReleaseKnob_.setBounds (knobRow);
```

In `AlgorithmSelector::resized()`:
```cpp
prevButton_.setBounds(b.removeFromLeft(14));   // was 20
nextButton_.setBounds(b.removeFromRight(14));  // was 20
```

In `AlgorithmSelector::AlgorithmSelector()`:
```cpp
nameLabel_.setFont(juce::Font(MLIMColours::kFontSizeSmall, juce::Font::bold));  // was kFontSizeMedium
```

## Dependencies
Requires task 293 (for the ADVANCED slot width)
