# Task 283: Clean Up Stale M-LIM/tasks/ Directory

## Description
The `M-LIM/tasks/` subdirectory is a leftover from an earlier project structure when tasks
were tracked inside the plugin source directory. All task management has since moved to the
root `/workspace/tasks/` directory. The stale files in `M-LIM/tasks/` are:

- Never used by workers (workers read from root `tasks/`)
- Cause confusion when searching for tasks
- Some have conflicting or superseded content vs. root tasks/

Files to remove from git:
- `M-LIM/tasks/pending/195-loudness-meter-mono-lra-edge-cases.md`
- `M-LIM/tasks/pending/196-limiter-engine-assertion-and-edge-cases.md`
- `M-LIM/tasks/pending/198-transient-leveling-parameter-range-tests.md`
- `M-LIM/tasks/pending/199-undo-manager-edge-cases.md`
- `M-LIM/tasks/pending/200-ab-state-edge-cases.md`
- `M-LIM/tasks/pending/201-dc-filter-dither-edge-cases.md`
- `M-LIM/tasks/pending/202-sidechain-filter-edge-cases.md`
- `M-LIM/tasks/pending/203-preset-manager-navigation-edge-cases.md`
- `M-LIM/tasks/pending/204-algorithm-selector-single-button-style.md`
- `M-LIM/tasks/pending/205-advanced-vertical-side-tab.md`
- `M-LIM/tasks/pending/206-knob-face-flat-disc-appearance.md`
- `M-LIM/tasks/pending/270-waveform-scale-strip-right-side-overlay.md` (superseded by task 276)
- `M-LIM/tasks/pending/271-level-meter-track-segment-texture.md` (ported to task 282)
- `M-LIM/tasks/pending/272-rmse-re-measure-post-visual-tasks.md` (superseded by task 281)
- `M-LIM/screenshots/task-256-rmse-results.txt` (duplicate of root screenshots/)

Also add `M-LIM/tasks/` to `.gitignore` (or remove the directory entirely if empty after cleanup).

## Produces
None

## Consumes
None

## Relevant Files
Skip: `M-LIM/src/` — do not touch plugin source
Skip: `tasks/` — root tasks directory, do not modify

## Acceptance Criteria
- [ ] Run: `ls M-LIM/tasks/pending/ 2>/dev/null | wc -l` → Expected: `0` (all stale files removed)
- [ ] Run: `git status` → Expected: no untracked M-LIM/tasks/ files
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: build still succeeds (cleanup doesn't affect source)

## Tests
None

## Technical Details
```bash
git rm -r M-LIM/tasks/
git rm M-LIM/screenshots/task-256-rmse-results.txt
# If M-LIM/tasks/ dir was only these files, it will be removed automatically
# Otherwise add to .gitignore:
echo "M-LIM/tasks/" >> .gitignore
git add .gitignore
git commit -m "cleanup: remove stale M-LIM/tasks/ directory (superseded by root tasks/)"
```

Note: Do NOT run this if workers are actively claiming from M-LIM/tasks/. Verify
`M-LIM/tasks/active/` is empty first.

## Dependencies
Requires tasks 281 (RMSE re-measure complete — confirms all visual work is captured in root tasks)
