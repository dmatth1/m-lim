# Task: Remove Dead Legacy Members from AlgorithmSelector

## Description
`AlgorithmSelector.h` (lines 45-47) declares three unused members:

```cpp
juce::TextButton prevButton_;
juce::TextButton nextButton_;
juce::Label nameLabel_;
```

In the constructor, these are made invisible but never used:
```cpp
prevButton_.setVisible(false);
nextButton_.setVisible(false);
nameLabel_.setVisible(false);
```

These are vestiges of the original navigation-style selector that was replaced with a button row (tasks 229, 278, 300). They serve no purpose and confuse readers about the component's actual structure.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/AlgorithmSelector.h` — remove prevButton_, nextButton_, nameLabel_ declarations
Modify: `M-LIM/src/ui/AlgorithmSelector.cpp` — remove setVisible(false) calls and any other references

## Acceptance Criteria
- [ ] Run: `grep -n "prevButton_\|nextButton_\|nameLabel_" src/ui/AlgorithmSelector.h src/ui/AlgorithmSelector.cpp` → Expected: no output
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds

## Tests
None

## Technical Details
Trivial removal — grep for all references to these three members across both files before deleting to ensure nothing else references them.

## Dependencies
None
