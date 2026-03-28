# Task: Move LockFreeFIFO into Its Own Header

## Description
`LockFreeFIFO<T>` is a general-purpose single-producer/single-consumer ring buffer template,
but it is defined inside `MeterData.h` (lines 37–113) — a domain-specific header for audio
metering structs.

This creates a cohesion violation:
- Code that only needs a lock-free FIFO (e.g., tests, future non-metering use cases) must include
  `MeterData.h` and pull in the `MeterData` struct, `juce_audio_basics`, etc.
- The current coupling hides the FIFO's general utility and makes it harder to discover and test
  independently.
- `tests/dsp/test_lockfree_fifo.cpp` already exists as an independent test file, confirming the
  FIFO is a self-contained utility that deserves a standalone header.

Move `LockFreeFIFO` into `M-LIM/src/dsp/LockFreeFIFO.h` and update `MeterData.h` to include it.

## Produces
None

## Consumes
None

## Relevant Files
Create: `M-LIM/src/dsp/LockFreeFIFO.h` — contains only the `LockFreeFIFO<T>` template (cut from MeterData.h)
Modify: `M-LIM/src/dsp/MeterData.h` — replace the `LockFreeFIFO` class body with `#include "LockFreeFIFO.h"`
Read: `M-LIM/tests/dsp/test_lockfree_fifo.cpp` — update include if needed

## Acceptance Criteria
- [ ] Run: `grep -c "LockFreeFIFO" M-LIM/src/dsp/MeterData.h` → Expected: 1 (only the include, no class body)
- [ ] Run: `grep -c "class LockFreeFIFO" M-LIM/src/dsp/LockFreeFIFO.h` → Expected: 1
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (existing `tests/dsp/test_lockfree_fifo.cpp` covers the FIFO logic)

## Technical Details
- `LockFreeFIFO.h` should include only `<atomic>` and `<vector>` (no JUCE headers needed)
- `MeterData.h` should keep its existing `#include <atomic>` etc. and add `#include "LockFreeFIFO.h"`
- No CMakeLists.txt changes are required since headers are not listed as source files
- All existing `#include "MeterData.h"` sites automatically gain access to `LockFreeFIFO` via
  the transitive include — no other files need updating

## Dependencies
None
