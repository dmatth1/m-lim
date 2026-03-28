# Task: Output Meter Needs LED-Segment Styling with Color Zones

## Description
The output meter (rightmost stereo bars) in Pro-L 2 shows distinct LED-style segments with clear color zones: white/light-gray segments in the safe zone, yellow/warm segments in the mid zone, and red segments at the top (danger/clip). The reference (video frame v1-0002, prol2-main-ui.jpg) shows the output meter as a tall bar with visible horizontal LED-style separations and distinct white-to-yellow-to-red color banding. Our output meter renders as a continuous fill with subtle segment separators but lacks the distinct multi-zone color banding that makes the reference meter visually striking.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/ui/LevelMeter.cpp` — current meter rendering, color zones, segment separators
Read: `src/ui/Colours.h` — meterSafe, meterWarning, meterDanger colors
Read: `/reference-docs/video-frames/v1-0002.png` — clear reference of output meter with LED segments
Read: `/reference-docs/reference-screenshots/prol2-metering.jpg` — detailed meter view

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM && export CCACHE_DIR=/build-cache/ccache && cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: screenshot of output meter area → Expected: visible LED-style segments with white/yellow/red color zones matching reference

## Tests
None

## Technical Details
- The reference output meter has approximately 40-50 visible LED segments per bar
- Color zones in reference: white/bright segments (lower ~80% safe zone), yellow (warning ~-3 to -1 dB), red (top ~-1 to 0 dB)
- Our LevelMeter already has segment separators (drawSegmentSeparators) and color zones, but the visual impact may be insufficient
- The segment height/gap ratio and color transition thresholds may need tuning
- The reference segments appear as distinct bright rectangles separated by dark gaps — more like real LED meters
- Segment height should be approximately 3px with 1px dark gap (already configured but may need visual verification)
- The white/bright color for safe zone segments is key — reference shows near-white, not gray-blue

## Dependencies
None
