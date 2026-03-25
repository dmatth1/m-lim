# Task 079: CMakeLists.txt Missing POSITION_INDEPENDENT_CODE for MLIMLib

## Description
`MLIMLib` is a static library that is linked into the VST3 shared object (`.so`). On Linux, static libraries linked into shared objects must be compiled with `-fPIC` (Position Independent Code). Without it, the linker fails with:

```
/usr/bin/ld: libMLIMLib.a: relocation R_X86_64_PC32 against symbol
can not be used when making a shared object; recompile with -fPIC
```

This `POSITION_INDEPENDENT_CODE` property was missing from the `MLIMLib` target definition in `CMakeLists.txt`.

**NOTE:** This was already fixed directly in `CMakeLists.txt` (trivial one-line fix). This task file documents the fix and adds a CI verification step.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/CMakeLists.txt` — verify `set_property(TARGET MLIMLib PROPERTY POSITION_INDEPENDENT_CODE ON)` is present

## Acceptance Criteria
- [ ] Run: `grep "POSITION_INDEPENDENT_CODE" M-LIM/CMakeLists.txt` → Expected: line is present
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: `[100%] Built target MLIM_VST3`

## Tests
None (build infrastructure fix)

## Technical Details
The fix that was applied:
```cmake
# After add_library(MLIMLib STATIC ...) and before target_link_libraries(MLIMLib ...)
set_property(TARGET MLIMLib PROPERTY POSITION_INDEPENDENT_CODE ON)
```

This is required because:
- VST3 format = shared object (`.so`) on Linux
- Static lib linked into shared lib must be PIC-compiled
- JUCE's own modules are compiled with PIC already (they're part of the plugin target)
- Only `MLIMLib` was missing this property

## Dependencies
None — already fixed
