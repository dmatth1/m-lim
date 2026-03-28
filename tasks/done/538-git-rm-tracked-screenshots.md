# Task 538: Remove Tracked Screenshot PNGs from Git

## Description
19 PNG screenshot files (1.7 MB total) are tracked in git despite `.gitignore` rules explicitly excluding them. These were committed before the ignore rules were established. They should be removed from git tracking while preserving local copies.

Affected directories:
- `screenshots/*.png` (9 files, ~1.1 MB)
- `M-LIM/screenshots/*.png` (10 files, ~0.6 MB)

The `.gitignore` already has rules for these paths, so after `git rm --cached` they will stay ignored.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `screenshots/` — `git rm --cached` all .png files
Modify: `M-LIM/screenshots/` — `git rm --cached` all .png files
Read: `.gitignore` — verify ignore rules are already in place

## Acceptance Criteria
- [ ] Run: `git ls-files '*.png' | wc -l` → Expected: 0
- [ ] Run: `ls screenshots/*.png 2>/dev/null | wc -l` → Expected: files still exist locally (not deleted from disk)
- [ ] Run: `git status --short screenshots/` → Expected: no tracked PNG files

## Tests
None

## Technical Details
```bash
git rm --cached screenshots/*.png M-LIM/screenshots/*.png
git commit -m "Remove tracked screenshot PNGs (already in .gitignore)"
```

## Dependencies
None
