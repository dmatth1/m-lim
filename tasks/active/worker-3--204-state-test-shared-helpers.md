# Task 204: Extract Shared Test Helpers for State Tests

## Description
Three state test files duplicate the same test infrastructure:

| File | Stub class name | Duplication |
|------|----------------|-------------|
| `test_ab_state.cpp` | `ABTestProcessor` | 15-line createLayout(), getParam(), setParam() |
| `test_preset_manager.cpp` | `TestProcessor` | identical 15-line createLayout() |
| `test_preset_manager_errors.cpp` | `ErrorTestProcessor` | identical 15-line createLayout() |

Additionally, `test_preset_manager.cpp` and `test_preset_manager_errors.cpp` each define their own RAII temp-directory helper (`TempPresetDir` / `TempDir`).

Create a shared header `tests/state/state_test_helpers.h` that provides:
1. A single `StateTestProcessor` stub class (rename all three per-file stubs to use this)
2. A single `getParam(apvts, id)` / `setParam(apvts, id, value)` helper pair
3. A single `TempPresetDir` RAII class

Then update the three test files to `#include "state_test_helpers.h"` and remove the duplicated definitions.

## Produces
None

## Consumes
None

## Relevant Files
Create: `M-LIM/tests/state/state_test_helpers.h` — shared test infrastructure
Modify: `M-LIM/tests/state/test_ab_state.cpp` — remove ABTestProcessor, use StateTestProcessor
Modify: `M-LIM/tests/state/test_preset_manager.cpp` — remove TestProcessor + TempPresetDir, use shared
Modify: `M-LIM/tests/state/test_preset_manager_errors.cpp` — remove ErrorTestProcessor + TempDir, use shared

## Acceptance Criteria
- [ ] Run: `grep -c "struct.*Processor\|class.*Processor" M-LIM/tests/state/test_ab_state.cpp M-LIM/tests/state/test_preset_manager.cpp M-LIM/tests/state/test_preset_manager_errors.cpp` → Expected: 0 (all definitions moved to helper header)
- [ ] Run: `cd build && ctest -R "ab_state|preset" --output-on-failure 2>&1 | tail -10` → Expected: all state tests pass

## Tests
None (this task is purely infrastructure refactoring; tests themselves remain unchanged)

## Technical Details
- `StateTestProcessor` needs a default constructor that sets up an APVTS with at least the "gain" and "ceiling" parameters (to satisfy existing test assertions).
- Keep `createLayout()` logic identical to the current per-file versions — do not change parameter ranges.
- `TempPresetDir` should use a random suffix (as current implementations do) and delete itself in its destructor.
- Include guards or `#pragma once` in the new header.

## Dependencies
None
