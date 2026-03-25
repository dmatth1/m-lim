# Task 035: Fix Editor LookAndFeel Destruction Order

## Description
Task 027 specifies applying MLIMLookAndFeel in the editor constructor and resetting in the destructor. However, JUCE requires that the LookAndFeel object outlives all components that reference it. If the LookAndFeel is a member of the editor, it may be destroyed before child components (C++ destroys members in reverse declaration order). This causes undefined behavior or crashes. The destructor must explicitly clear the LookAndFeel from all child components before they are destroyed.

## Produces
None

## Consumes
EditorCore
LookAndFeelDefinition

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — ensure MLIMLookAndFeel is declared BEFORE all child component members
Modify: `M-LIM/src/PluginEditor.cpp` — in destructor: call `setLookAndFeel(nullptr)` before child destruction
Read: `M-LIM/src/ui/LookAndFeel.h` — LookAndFeel class

## Acceptance Criteria
- [ ] Run: `cd M-LIM && grep -n "setLookAndFeel.*nullptr" src/PluginEditor.cpp` → Expected: at least one line setting LookAndFeel to nullptr in destructor
- [ ] Run: `cd M-LIM && head -50 src/PluginEditor.h | grep -B2 -A2 "LookAndFeel"` → Expected: MLIMLookAndFeel member declared before component members

## Tests
- Integration: `tests/integration/test_plugin_processor.cpp::test_editor_create_destroy_no_crash` — create and destroy editor multiple times without crash or assertion

## Technical Details
- In destructor, BEFORE any child components are destroyed: `setLookAndFeel(nullptr);`
- Declare the LookAndFeel member as the FIRST non-trivial member in the editor class, so it's destroyed last
- Alternatively, make the LookAndFeel a `SharedResourcePointer<MLIMLookAndFeel>` owned by the processor or as a static — but simplest fix is declaration order + explicit nullptr
- This is a common JUCE pitfall — many plugins crash on close due to this exact issue

## Dependencies
Requires tasks 003, 027
