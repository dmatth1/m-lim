# Task 260: GainReductionMeter — Remove Input Level Background Bars

## Description
`GainReductionMeter::drawInputLevel()` draws left/right input level bars rising from the bottom of
the GR meter as a background layer, then the GR red bar is drawn on top. The reference Pro-L 2 GR
meter shows **only the gain reduction bar** — there are no input level background bars visible in the
GR meter column. The input level is shown exclusively in the separate left/right `LevelMeter`
components.

The composite input-level-behind-GR rendering adds visual noise and differs from the reference.
The fix is simple: remove the `drawInputLevel()` call from `GainReductionMeter::paint()`.

Also: the `setInputLevel()` method and related members (`inputLevelL_`, `inputLevelR_`,
`inputLevelToFrac()`, `drawInputLevel()`, `kInputMinDB`, `kInputMaxDB`) are then unused and should
be removed to keep the class clean.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/ui/GainReductionMeter.cpp` — remove `drawInputLevel()` call from `paint()`, remove
  `drawInputLevel()` implementation and `setInputLevel()` implementation
Modify: `src/ui/GainReductionMeter.h` — remove `setInputLevel()` declaration, `inputLevelL_/R_`
  members, `kInputMinDB`, `kInputMaxDB`, `inputLevelToFrac()`, `drawInputLevel()` declarations
Modify: `src/PluginEditor.cpp` — remove the `grMeter_.setInputLevel(inL, inR)` call in
  `applyMeterData()`
Read:   `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — confirm GR meter shows only GR bar

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -5` →
      Expected: build succeeds with no warnings about unused members
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` →
      Expected: all tests pass

## Tests
None

## Technical Details
In `GainReductionMeter::paint()`, replace:
```cpp
drawInputLevel (g, barArea);
drawBar        (g, barArea);
```
with just:
```cpp
drawBar (g, barArea);
```

Then remove `drawInputLevel()` entirely from both .h and .cpp.

In `GainReductionMeter.h`, remove:
- `void setInputLevel (float leftDB, float rightDB);` declaration
- `float inputLevelL_ = -96.0f;`
- `float inputLevelR_ = -96.0f;`
- `static constexpr float kInputMinDB = -30.0f;`
- `static constexpr float kInputMaxDB =   0.0f;`
- `void drawInputLevel (...) const;` private declaration
- `float inputLevelToFrac (float dBFS) const noexcept;` declaration

In `src/PluginEditor.cpp`, remove:
```cpp
grMeter_.setInputLevel (inL, inR);
```

## Dependencies
None
