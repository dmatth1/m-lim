# Task 239: Algorithm Selector — Fix Truncated Button Labels

## Description
The 8 algorithm buttons in the STYLE section of the control strip display truncated text
("Tran...", "Dyna...", "Aggr...", "Allro...", "Mod...") because the button width
(~55 px at default 900 px window) is too narrow for the full algorithm names defined in
`kAlgorithmNames[]` in `LimiterAlgorithm.h`.

**Fix**: In `AlgorithmSelector.cpp`, define a separate array of short button labels that
fit in ~55 px at font size 10 pt (≤6 chars each), and use those short labels for button
display. The full names in `kAlgorithmNames[]` must remain unchanged for APVTS/preset
serialisation — only the visible button text changes.

Proposed short labels (max 6 chars):
| Algorithm    | Full name    | Short label |
|-------------|--------------|-------------|
| 0           | Transparent  | Trans       |
| 1           | Punchy       | Punch       |
| 2           | Dynamic      | Dyn         |
| 3           | Aggressive   | Aggr        |
| 4           | Allround     | Allrnd      |
| 5           | Bus          | Bus         |
| 6           | Safe         | Safe        |
| 7           | Modern       | Modern      |

Set button text to the short label in the `AlgorithmSelector` constructor loop (replace
`kAlgorithmNames[i]` with `kAlgorithmButtonLabels[i]`). Keep the existing APVTS
ComboBox using `kAlgorithmNames` for parameter serialisation.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/AlgorithmSelector.cpp` — change button label text from `kAlgorithmNames[i]` to short labels
Read:   `src/ui/AlgorithmSelector.h` — class structure and button members
Read:   `src/dsp/LimiterAlgorithm.h` — kAlgorithmNames[] must not be changed

## Acceptance Criteria
- [ ] Run: build, launch standalone, observe STYLE button row → Expected: all 8 buttons show full readable text (no "..." truncation) at 900 px window width
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (visual fix only; no testable logic)

## Technical Details
In `AlgorithmSelector.cpp` add at file scope:
```cpp
static constexpr const char* kAlgorithmButtonLabels[] = {
    "Trans", "Punch", "Dyn", "Aggr", "Allrnd", "Bus", "Safe", "Modern"
};
```
Then in the constructor loop change:
```cpp
algoButtons_[i]->setButtonText(kAlgorithmNames[i]);
```
to:
```cpp
algoButtons_[i]->setButtonText(kAlgorithmButtonLabels[i]);
```

## Dependencies
None
