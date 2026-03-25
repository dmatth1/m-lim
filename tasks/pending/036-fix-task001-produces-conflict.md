# Task 036: Fix Task 001 / Task 017 Produces Conflict

## Description
Tasks 001 and 017 both declare `Produces: PluginProcessorCore`. Task 001 creates stub files (empty processBlock, no DSP), while task 017 wires the real implementation. Having two tasks produce the same interface confuses the dependency system — downstream tasks can't tell which one actually provides the working interface.

Fix: Task 001 should produce a build artifact (`artifact:M-LIM/build`) or a setup-specific marker, not `PluginProcessorCore`. Only task 017 should produce `PluginProcessorCore` since it's the one that implements the real interface.

This is a task file metadata fix — no code changes needed.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `tasks/pending/001-project-setup.md` — change Produces from `PluginProcessorCore` to `artifact:M-LIM/build` or a new `ProjectSkeleton` interface
Read: `tasks/pending/017-plugin-processor-integration.md` — verify it properly claims PluginProcessorCore

## Acceptance Criteria
- [ ] Run: `grep -A1 "## Produces" tasks/pending/001-project-setup.md` → Expected: does NOT say PluginProcessorCore
- [ ] Run: `grep -A1 "## Produces" tasks/pending/017-plugin-processor-integration.md` → Expected: says PluginProcessorCore

## Tests
None (metadata fix)

## Technical Details
- Task 001 produces: the project skeleton (CMakeLists.txt, stub files, submodules, directory structure)
- Task 017 produces: the working PluginProcessorCore (real processBlock, state save/load, latency reporting)
- Suggested: Task 001 Produces: `artifact:M-LIM/build` (the CMake build directory exists after configure)
- All tasks that depend on "project exists and compiles" should depend on task 001
- All tasks that depend on "processor actually processes audio" should depend on task 017

## Dependencies
None
