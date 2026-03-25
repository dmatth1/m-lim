# Task 141: Fix warning zone draw condition typo in LevelMeter::drawChannel()

## Description
`LevelMeter.cpp` line 107 has a malformed condition in the warning zone draw block:

```cpp
if (fillTop < warnBot && dangerBot < barTop + fillH + barTop)
```

The expression `barTop + fillH + barTop` adds `barTop` twice. The intended check is
`dangerBot < barTop + barH` (i.e., danger zone is within the bar — always true and therefore the
second clause is vacuous), **or** more likely the condition should simply be removed since
`fillTop < warnBot` alone is the correct and sufficient guard.

The double-addition means: when `barTop > 0` (e.g. if bar rectangles are ever translated) the
condition `dangerBot < 2*barTop + fillH` can incorrectly suppress warning zone rendering for
low-level signals. In the current code `barTop` is always 0, so there is no visible bug, but the
intent is wrong and the condition is confusing.

Fix: simplify the guard to just the first condition (`fillTop < warnBot`), removing the redundant
and incorrect second clause.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LevelMeter.cpp` — fix line 107

## Acceptance Criteria
- [ ] Run: `grep -n "barTop + fillH + barTop" M-LIM/src/ui/LevelMeter.cpp` → Expected: no output
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | grep -E "error:"` → Expected: no output
- [ ] Visual: warning zone (orange band) still visible at levels between -3 dB and -0.5 dB in the level meter

## Tests
None

## Technical Details
Change line 107 from:
```cpp
if (fillTop < warnBot && dangerBot < barTop + fillH + barTop)
```
To:
```cpp
if (fillTop < warnBot)
```
The inner `if (top < bot)` guard already prevents zero-height rects from being drawn.

## Dependencies
None
