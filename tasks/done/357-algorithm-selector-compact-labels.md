# Task 357: Algorithm Selector Compact Button Labels

## Description

The AlgorithmSelector uses a 2×4 grid of TextButtons with full algorithm names:
`"TRANSPARENT"`, `"PUNCHY"`, `"DYNAMIC"`, `"AGGRESSIVE"`, `"ALLROUND"`, `"BUS"`, `"SAFE"`, `"MODERN"`.

Each button is ~37px wide (150px slot ÷ 4 columns). With the 9pt font, names like
"TRANSPARENT" (~66px wide) don't fit and are shown as truncated `"T..."` in the screenshot.
This is visually degraded compared to Pro-L 2's clean button labels.

Fix: replace the long names with short 2–4 character abbreviations that fit comfortably
in the narrow buttons.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/AlgorithmSelector.cpp` — `kAlgorithmButtonLabels[]` array

## Acceptance Criteria
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -c "error:"` → Expected: `0`
- [ ] Run: visual inspection of screenshot — all 8 algorithm buttons should show readable text
  without truncation ("..." suffix must not appear on any button label)
- [ ] Run: full image RMSE → Expected: ≤ 22.08% (no regression, ideally slight improvement)

## Tests
None

## Technical Details

In `src/ui/AlgorithmSelector.cpp`, change `kAlgorithmButtonLabels[]` from:
```cpp
static constexpr const char* kAlgorithmButtonLabels[] = {
    "TRANSPARENT", "PUNCHY", "DYNAMIC", "AGGRESSIVE",
    "ALLROUND", "BUS", "SAFE", "MODERN"
};
```
to:
```cpp
static constexpr const char* kAlgorithmButtonLabels[] = {
    "TR", "PU", "DY", "AG",
    "AR", "BU", "SA", "MO"
};
```

2-character labels guarantee fitting in any button width ≥ 20px. If the worker prefers
slightly longer names (e.g. "TRNSP", "PUNCH") that still fit without truncation, that is
acceptable — but verify in the screenshot that no "..." appears.

Note: the `nameLabel_` (which shows current algorithm name in the legacy nav widget) is
already hidden (`nameLabel_.setVisible(false)`), so changing `kAlgorithmButtonLabels` does
not break any visible display path other than the buttons themselves.

## Dependencies
None
