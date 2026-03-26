# Task 295: TopBar Height — Reduce from 30 px to 24 px

## Description
The current `kTopBarH = 30` in PluginEditor.h gives M-LIM a top bar that is taller than in the
Pro-L 2 reference. The reference top bar is approximately 22-25px tall (measured at 900x500 scale
from `prol2-main-ui.jpg`). M-LIM's 30px bar wastes vertical space and contributes to the 31.08%
RMSE in the top bar sub-region.

**Fix**: Reduce `kTopBarH` from 30 to 24. This gives 6px more height to the waveform display,
moves the control strip up by 6px relative to the top, and makes the overall proportions closer to
the reference.

The TopBar component itself should scale its content (logo, buttons) to fit the new height. If any
child buttons or labels have hard-coded heights that exceed 24px, they need to be reduced
proportionally.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — change `kTopBarH` from 30 to 24
Read: `M-LIM/src/ui/TopBar.h` — check if any fixed-height children exceed 24px
Read: `M-LIM/src/ui/TopBar.cpp` — verify button/label layout uses proportional sizing vs fixed heights

## Acceptance Criteria
- [ ] Run: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone` → Expected: exits 0
- [ ] Visual: Screenshot shows top bar is visually thinner — approximately matching the reference height proportionally (about 4-5% of total plugin height vs current 6%)
- [ ] Visual: All TopBar buttons and labels remain fully visible within the 24px height — no clipping

## Tests
None

## Technical Details
Change in `PluginEditor.h`:
```cpp
static constexpr int kTopBarH = 24;  // was 30; reference ~22-25px at 900x500 scale
```

The TopBar component uses `getLocalBounds()` in `resized()` and should scale automatically.
Verify that RotaryKnob labels or any fixed-font-size elements in TopBar don't overflow the 24px
height.

## Dependencies
None
