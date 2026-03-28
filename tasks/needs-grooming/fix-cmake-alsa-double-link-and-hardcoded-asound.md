# Task: Fix CMakeLists.txt ALSA double-link and hardcoded asound in tests

## Description
Two CMake build issues related to ALSA:

**Issue 1 — ALSA linked twice to MLIMLib (root CMakeLists.txt)**

`CMakeLists.txt` lines 66–78 add `${ALSA_LIBRARIES}` to `MLIMLib` twice:
- Line 73: via generator expression `$<$<BOOL:${ALSA_FOUND}>:${ALSA_LIBRARIES}>`
  inside the primary `target_link_libraries(MLIMLib PUBLIC ...)` call
- Line 77: explicitly again inside `if(ALSA_FOUND AND UNIX AND NOT APPLE)`

Both conditions fire on the same Linux system when ALSA is found, so ALSA is linked
twice. This can produce linker warnings (`multiple definition`) and bloats the link
command.

**Fix**: Remove the redundant explicit block (lines 76–78) and keep only the generator
expression on line 73:
```cmake
# Delete these lines:
if(ALSA_FOUND AND UNIX AND NOT APPLE)
    target_link_libraries(MLIMLib PUBLIC ${ALSA_LIBRARIES})
endif()
```

**Issue 2 — ALSA hardcoded as `asound` in tests/CMakeLists.txt (line 71)**

`tests/CMakeLists.txt` links `asound` directly rather than using the CMake ALSA
find-package result:
```cmake
target_link_libraries(MLIMTests
    PRIVATE
        MLIMLib
        asound       # ← hardcoded
)
```

This bypasses the CMake `find_package(ALSA)` result and will break on systems where
ALSA has a different name (e.g., cross-compilation targets, or when ALSA is absent and
`ALSA_FOUND` is false). The correct pattern is to use the conditional generator
expression already used by MLIMLib.

**Fix**:
```cmake
target_link_libraries(MLIMTests
    PRIVATE
        MLIMLib
        $<$<BOOL:${ALSA_FOUND}>:${ALSA_LIBRARIES}>
)
```

Note: `MLIMLib` already links ALSA transitively via its PUBLIC dependencies, so the
explicit ALSA link in MLIMTests may be entirely redundant after issue 1 is fixed. If
the build succeeds without it, the ALSA line can be removed entirely from
`tests/CMakeLists.txt`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `CMakeLists.txt` — remove duplicate explicit ALSA target_link_libraries block
  (lines 76–78)
Modify: `tests/CMakeLists.txt` — replace hardcoded `asound` with
  `$<$<BOOL:${ALSA_FOUND}>:${ALSA_LIBRARIES}>` (or remove entirely if redundant)

## Acceptance Criteria
- [ ] Run: `cmake -B build_alsa_test -DCMAKE_BUILD_TYPE=Release 2>&1 | grep -i "warning\|ALSA"` → Expected: no "multiple definition" or duplicate ALSA warnings
- [ ] Run: `cmake --build build --target MLIMTests -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, no linker warnings about multiple ALSA references
- [ ] Run: `cd build && ctest -R MLIMTests --output-on-failure 2>&1 | tail -5` → Expected: all tests pass

## Tests
None (CMake build system fix — testable only via build success)

## Technical Details
The ALSA `find_package` is done at the top of `CMakeLists.txt` (line 17–19). Both
`ALSA_FOUND` and `ALSA_LIBRARIES` are in scope throughout the file and in
`tests/CMakeLists.txt` (it is an `add_subdirectory` so it inherits parent scope).

## Dependencies
None
