# Task 222: Remove Committed Screenshot PNGs from Git History

## Description
Four screenshot PNG files are tracked in git under `M-LIM/screenshots/`:
- `M-LIM/screenshots/after-gr-fix.png` (55K)
- `M-LIM/screenshots/current-ui.png` (55K)
- `M-LIM/screenshots/task-151-after.png` (27K)
- `M-LIM/screenshots/task-151-after_000.png` (27K)

These are build/test artifacts — UI screenshots taken during development — and should not be permanently committed. The root `.gitignore` already has `screenshots/*.png` which covers these paths, but they are already tracked so gitignore doesn't remove them.

Remove these files from git tracking without deleting them from the working tree (they are transient artifacts and can be regenerated). Also verify the `.gitignore` patterns cover `M-LIM/screenshots/` so future screenshots are not accidentally staged.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `.gitignore` — verify `screenshots/*.png` matches `M-LIM/screenshots/` (gitignore without leading slash matches anywhere in the tree, so it should already work)

## Acceptance Criteria
- [ ] Run: `git ls-files M-LIM/screenshots/` → Expected: empty output (no tracked files)
- [ ] Run: `git status M-LIM/screenshots/` → Expected: no changes shown (files either absent or listed as ignored)
- [ ] Run: `git check-ignore -v M-LIM/screenshots/task-151-after.png` → Expected: `.gitignore:N:screenshots/*.png M-LIM/screenshots/task-151-after.png` (or similar showing the pattern matches)

## Tests
None

## Technical Details
```bash
# Remove from git tracking (keeps local file)
git rm --cached M-LIM/screenshots/after-gr-fix.png
git rm --cached M-LIM/screenshots/current-ui.png
git rm --cached M-LIM/screenshots/task-151-after.png
git rm --cached M-LIM/screenshots/task-151-after_000.png

# Commit the removal
git commit -m "chore: remove committed screenshot artifacts from git tracking"
```

If the `.gitignore` pattern `screenshots/*.png` does not match the `M-LIM/screenshots/` subdirectory path, add an explicit pattern:
```
M-LIM/screenshots/*.png
M-LIM/screenshots/*.jpg
```

## Dependencies
None
