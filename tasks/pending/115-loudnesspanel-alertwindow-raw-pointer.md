# Task 115: LoudnessPanel AlertWindow Uses Raw new/delete — Replace with RAII

## Description
`LoudnessPanel.cpp` (lines 46–68) allocates a `juce::AlertWindow` with a raw
`new` and manually calls `delete alertWindow` inside a modal callback:

```cpp
auto* alertWindow = new juce::AlertWindow ("Custom Target", ..., ...);
...
juce::ModalCallbackFunction::forComponent (
    [alertWindow](int result, LoudnessPanel* self)
    {
        ...
        delete alertWindow;   // ← manual delete
    }, this);
```

Problems:
1. If the component is destroyed before the modal callback fires (e.g. plugin
   window closed while dialog is open), `self` becomes a dangling pointer.
2. Manual `delete` is easy to miss in new code paths.
3. JUCE provides `juce::DialogWindow::LaunchOptions` or
   `juce::AlertWindow::showOkCancelBox` which handle lifetime automatically.

Fix: Replace the raw `new`/`delete` pattern with
`juce::AlertWindow::showInputBox` (or equivalent JUCE async API) that manages
its own lifetime and delivers the result via a `std::function` callback.  If the
component is destroyed before the callback fires, use a `juce::Component::SafePointer<LoudnessPanel>`
guard inside the lambda so that the callback is a no-op on a dead component.

Steps:
1. Refactor the `targetButton_.onClick` lambda to use a JUCE-managed dialog.
2. Guard the result-handling closure with
   `juce::Component::SafePointer<LoudnessPanel> safeThis (this)` and check
   `if (safeThis == nullptr) return;` before acting on the result.
3. Remove all `new juce::AlertWindow` and `delete alertWindow` lines.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/LoudnessPanel.cpp` — replace raw AlertWindow lifecycle

## Acceptance Criteria
- [ ] Run: `grep -n "new juce::AlertWindow\|delete alertWindow" M-LIM/src/ui/LoudnessPanel.cpp` → Expected: no matches
- [ ] Run: `grep -n "SafePointer" M-LIM/src/ui/LoudnessPanel.cpp` → Expected: at least one match (dangling-pointer guard)
- [ ] Run: `cd M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds
- [ ] Run: `cd M-LIM && cd build && ctest --output-on-failure 2>&1 | tail -5` → Expected: all tests pass

## Tests
None — UI interaction change; no automated test for modal dialogs.

## Technical Details
- JUCE's `AlertWindow::showAsync` or `NativeMessageBox::showAsync` are the
  preferred RAII-safe APIs in JUCE 7.
- Alternatively, an `AlertWindow` can be held in a `std::unique_ptr` member on
  LoudnessPanel, destroyed in the component's destructor.
- Keep the existing validation logic (clamp value to [-60, 0] range) unchanged.

## Dependencies
None
