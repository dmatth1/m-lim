# Task: Fix Output Meter Excessive Idle Brightness

## Description
The output meter bars (rightmost vertical stereo bars) render too bright at idle. Pixel comparison at output meter bar center: M-LIM renders ~(143,143,151) while at idle the bars should show a dark track background. The output meter's `setIdleSimulationLevel(-0.5f)` in PluginEditor.cpp causes it to simulate a nearly-full signal at idle, which produces a bright steel-blue fill nearly to the top of the bars. In Pro-L 2, the output meter at rest shows dark/empty bars. The idle simulation level is too aggressive, making the output meter area appear washed out.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/PluginEditor.cpp` — line 26: `outputMeter_.setIdleSimulationLevel(-0.5f)` — change to a much lower idle level (e.g., -18.0f or remove idle simulation entirely)
Read: `src/ui/LevelMeter.cpp` — how idle simulation level affects the gradient fill
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — reference output meter at rest

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && export CCACHE_DIR=/build-cache/ccache && cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot and pixel sample at output meter bars → Expected: bars are mostly dark at idle, not bright gray

## Tests
None

## Technical Details
- Current: `setIdleSimulationLevel(-0.5f)` means the output meter simulates -0.5 dBFS signal at idle — nearly full scale, filling ~95% of the bar with color
- The reference output meter at idle/startup shows dark empty bars
- The idle simulation was likely added to make screenshots look more "alive" but -0.5 dBFS is too aggressive
- Consider removing idle simulation for the output meter entirely, or setting it to -24.0f or lower for a subtle idle appearance
- Note: there is an existing grooming task "increase-output-meter-idle-blue-fill.md" that says to make it BRIGHTER — that task appears to be comparing against the reference with active audio, not idle state. These two tasks conflict; the ProjectManager should reconcile them.

## Dependencies
None
