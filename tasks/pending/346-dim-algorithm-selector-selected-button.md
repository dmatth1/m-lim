# Task 346: Dim Algorithm Selector Selected-Button Color

## Description
The algorithm selector's **selected (on) button state** uses `accentBlue.withAlpha(0.8f)` which
renders as bright blue (~#2766BB) against the dark control strip background. Pixel sampling at
y=430, x=10-40 in the current 900×500 crop confirms this: M-LIM shows #2766BB while the
reference shows ~#413E47 (dark neutral gray).

The bright blue selected button is the most prominent **static** color mismatch remaining in the
control strip. Dimming it to a subtle highlight (darker, less-saturated blue) better matches the
Pro-L 2 reference control strip, which shows neutral-dark backgrounds in the algorithm area.

In `AlgorithmSelector.cpp` the color is set via:
```cpp
algoButtons_[i].setColour(juce::TextButton::buttonOnColourId,
                           MLIMColours::accentBlue.withAlpha(0.8f));
```

**Fix**: Replace `accentBlue.withAlpha(0.8f)` with a darker, less-saturated selection highlight.
Two options:
1. `MLIMColours::algoButtonInactive.brighter(0.6f)` — dark blue-gray with subtle lift
2. A new constant: `algoButtonSelected { 0xff2E3E58 }` in Colours.h (dark navy, ~50% darker
   than the current accent blue)

Recommended: option 2, add `algoButtonSelected = #2E3E58` to Colours.h and use it.

The selected button must still be **visually distinguishable** from inactive buttons (inactive =
`algoButtonInactive = #303848`). `#2E3E58` is brighter on the blue channel (0x58 vs 0x48) so
it reads as a subtle blue highlight rather than a bright accent.

## Produces
None

## Consumes
None

## Relevant Files
- Modify: `M-LIM/src/ui/AlgorithmSelector.cpp` — change `buttonOnColourId` color
- Modify: `M-LIM/src/ui/Colours.h` — add `algoButtonSelected` constant (optional)

## Acceptance Criteria
- [ ] `cmake --build M-LIM/build --config Release -j$(nproc) --target M-LIM_Standalone 2>&1 | tail -3` → exit 0
- [ ] Launch standalone, visually verify: selected algorithm button shows as subtle dark-navy
  highlight (not bright blue), while remaining distinguishable from un-selected buttons
- [ ] `source Scripts/ui-test-helper.sh && start_app && screenshot screenshots/task-346-after.png && stop_app`
- [ ] RMSE: `compare -metric RMSE screenshots/task-346-after.png <ref-cropped> /dev/null`
  → expected ≤ 22.05% (current baseline). Even a 0.1 pp improvement is a pass.
  Revert if RMSE increases.

## Tests
None

## Technical Details

Current pixel at y=430, x=20 (center of selected button):
- M-LIM: #92ADD0 (bright blue highlight inside selected button)
- Reference: #413E47 (dark neutral)

Color delta: ΔR=81, ΔG=111, ΔB=137 → significant blue/brightness excess.

With `algoButtonSelected = #2E3E58`:
- Expected rendered pixel ≈ #2E3E58 (dark navy)
- Remaining delta to reference #413E47: ΔR=19, ΔG=1, ΔB=17 — much smaller

Control strip sub-region is 900×92 = 82,800 px (18.4% of 450k). The selected button occupies
~30×50 = 1,500 px (0.33% of 450k). Pixel RMSE improvement at those pixels would be
(~50% → ~10%) = 40 pp reduction on 0.33% of pixels → ~0.13 pp full-image improvement.
Small but genuine.

## Dependencies
None
