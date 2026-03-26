# Task 238: Full UI Visual Verification — Build, Screenshot, Compare

## Description
Final verification task: build the plugin, launch it under Xvfb, take a screenshot of the running UI, and compare it visually against the FabFilter Pro-L 2 reference screenshot. Document the results with before/after screenshots.

Steps:
1. Install required tools if missing: `sudo apt-get install -y xvfb xdotool imagemagick scrot optipng bc`
2. Build the plugin: `cd /workspace/M-LIM && export CCACHE_DIR=/build-cache && cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER_LAUNCHER=ccache && cmake --build build -j$(nproc)`
3. Start Xvfb: `Xvfb :99 -screen 0 1280x800x24 &`
4. Set DISPLAY=:99 and launch the standalone: `/workspace/M-LIM/build/M-LIM_artefacts/Release/Standalone/M-LIM &`
5. Wait 3 seconds for the UI to appear
6. Take screenshot: `DISPLAY=:99 scrot -d 1 /workspace/screenshots/task-238-ui-final.png`
7. Stop the standalone app
8. Optimize screenshot: `optipng -o2 -fix -quiet /workspace/screenshots/task-238-ui-final.png`
9. Compare to reference using ImageMagick:
   ```
   convert /workspace/screenshots/task-238-ui-final.png -resize 900x500\! /tmp/current.png
   convert /reference-docs/reference-screenshots/prol2-main-ui.jpg -resize 900x500\! /tmp/reference.png
   compare -metric RMSE /tmp/current.png /tmp/reference.png /tmp/diff.png 2>&1
   ```
10. Log results to `/workspace/screenshots/task-238-results.txt`
11. Run the full test suite: `cd /workspace/M-LIM/build && ctest --output-on-failure`
12. Commit screenshots to git

## Produces
None

## Consumes
artifact:/workspace/M-LIM/build/M-LIM_artefacts/Release/Standalone/M-LIM

## Relevant Files
Read: `Scripts/visual-parity-auditor.sh` — if it exists, use it
Read: `Scripts/ui-test-helper.sh` — helper functions
Create: `screenshots/task-238-ui-final.png` — the screenshot
Create: `screenshots/task-238-results.txt` — comparison results

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && cmake --build build --config Release -j$(nproc) 2>&1 | tail -5` → Expected: exits 0
- [ ] Run: `ls /workspace/screenshots/task-238-ui-final.png` → Expected: file exists
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure -q 2>&1 | tail -10` → Expected: all tests pass

## Tests
None

## Technical Details
- If the standalone binary is at a different path, find it with: `find /workspace/M-LIM/build -name "M-LIM" -type f`
- The RMSE comparison will likely show a high value (>0.5) since the UIs won't be pixel-perfect — the goal is to document the current state
- Commit screenshots without optimization to save time: `git add screenshots/ && git commit -m "task 238: ui screenshot verification"`
- If Xvfb is already running on :99, skip starting it

## Dependencies
Requires tasks 229, 230, 231, 232, 233, 234, 235, 236, 237
