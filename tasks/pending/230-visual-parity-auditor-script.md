# Task 230: Create VisualParityAuditor Script

## Description
Create a shell script `Scripts/visual-parity-auditor.sh` that automates the full build+launch+screenshot+compare workflow for UI visual parity verification. This script is the main tool workers use to verify their UI changes against the FabFilter Pro-L 2 reference.

The script must:
1. Build the plugin (cmake, with ccache)
2. Start Xvfb on display :99
3. Source `Scripts/ui-test-helper.sh` to get helper functions
4. Launch the M-LIM standalone app using `start_app`
5. Capture a screenshot at `screenshots/parity-check-$(date +%Y%m%d-%H%M%S).png`
6. Compare against `/reference-docs/reference-screenshots/prol2-main-ui.jpg` using ImageMagick composite + rmse
7. Print PASS if RMSE < 0.40 (40%), FAIL otherwise — note that perfect pixel match is not expected, just rough layout/color similarity
8. Exit 0 on PASS, 1 on FAIL
9. Always stop Xvfb and the app on exit (use trap)

Also create `Scripts/ui-test-helper.sh` if it doesn't exist yet — it should define:
- `start_app()`: starts the standalone app binary on $DISPLAY :99
- `screenshot(filename)`: uses scrot or import to capture the screen
- `stop_app()`: kills the standalone app
- `compare_to_reference(ref, actual, threshold)`: runs ImageMagick compare

Install any missing dependencies (xvfb, scrot, imagemagick, bc) with apt-get if not present.

## Produces
artifact:Scripts/visual-parity-auditor.sh

## Consumes
artifact:/workspace/M-LIM/build/M-LIM_artefacts/Release/Standalone/M-LIM.app/Contents/MacOS/M-LIM

## Relevant Files
Read: `Scripts/ui-test-helper.sh` — check if it already exists, create if not
Create: `Scripts/visual-parity-auditor.sh` — the main auditor script
Read: `/reference-docs/reference-screenshots/prol2-main-ui.jpg` — primary reference image path
Read: `CLAUDE.md` — understand the build command and artifact paths

## Acceptance Criteria
- [ ] Run: `ls /workspace/Scripts/visual-parity-auditor.sh` → Expected: file exists
- [ ] Run: `bash -n /workspace/Scripts/visual-parity-auditor.sh` → Expected: exits 0 (valid bash syntax)
- [ ] Run: `ls /workspace/Scripts/ui-test-helper.sh` → Expected: file exists
- [ ] Run: `bash -n /workspace/Scripts/ui-test-helper.sh` → Expected: exits 0 (valid bash syntax)

## Tests
None

## Technical Details
- Standalone app binary path: `/workspace/M-LIM/build/M-LIM_artefacts/Release/Standalone/M-LIM` (Linux)
- Reference image: `/reference-docs/reference-screenshots/prol2-main-ui.jpg`
- Screenshots dir: `/workspace/screenshots/` (create if needed)
- Use `compare -metric RMSE` from ImageMagick for comparison
- Scale both images to same size before compare (e.g., 900x500)
- The script should be executable (chmod +x)

## Dependencies
None
