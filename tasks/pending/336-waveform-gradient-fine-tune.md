# Task 336: Waveform Gradient Colors — Fine-Tune Bottom to Match Reference

## Description
Follow-up to task 330 (fix waveform gradient top/bottom colors). After task 330 sets
displayGradientTop=#8992AB and displayGradientBottom=#5C6880, the bottom color may still
differ from the reference. Pixel sampling at y=370-380 (near-bottom of waveform area) in
the 900x500 reference shows:

  Reference at y=370-380, x=100-600: ~#7A809B to #7F84A2 (R=122-127, G=128-132, B=155-162)

If after task 330 the bottom color is still too dark, this task fine-tunes it further.

Cross-check from v1-0005.png (Pro-L 2 waveform closeup, background visible in middle):
  y=5   (top):    #99A9CC — lighter blue
  y=200 (mid):    #6F7790 — medium steel-blue
  y=280 (bottom): #565E76 — darker blue-gray

The reference bottom at ~#7A809B is lighter than v1-0005 bottom (#565E76), suggesting the
main reference shows a mix of background + output waveform content at the bottom. A reasonable
target is #687090.

Worker: First check what task 330 set for displayGradientBottom. If the RMSE remeasure
(task 339) shows waveform RMSE < 20%, this task may not be needed — skip or close it.
Only proceed if RMSE is still above 21%.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — fine-tune `displayGradientBottom` (and optionally top)
Read: `screenshots/task-339-rmse-results.txt` — check if RMSE improved enough with task 330 alone

## Acceptance Criteria
- [ ] Run: `cmake --build M-LIM/build --config Release -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds, exit 0
- [ ] Run: `source Scripts/ui-test-helper.sh && start_app && screenshot "screenshots/task-336-after.png" && stop_app` → Expected: screenshot saved
- [ ] Visual: Waveform gradient bottom is lighter/bluer than after task 330, closer to reference steel-blue

## Tests
None

## Technical Details
In `M-LIM/src/ui/Colours.h`, fine-tune gradient constants if needed:

```cpp
// Try brightening bottom toward reference's #7A809B:
const juce::Colour displayGradientBottom{ 0xff6878A8 };  // brighter steel-blue
```

Run RMSE before and after to confirm improvement. Revert if it makes things worse.

## Dependencies
Requires task 330 (fix waveform gradient colors — active) to be complete first.
