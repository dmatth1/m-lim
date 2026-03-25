#pragma once
/**
 * alloc_tracking.h — Shared allocation-tracking utilities for test builds.
 *
 * The global operator new / operator delete overrides are defined exactly once
 * in tests/integration/test_realtime_safety.cpp.  This header exposes the
 * thread-local counters and the AllocGuard RAII type so other test translation
 * units can participate in the same tracking without re-defining operator new.
 *
 * Usage:
 *   #include "alloc_tracking.h"
 *   ...
 *   AllocGuard guard;
 *   callSomething();
 *   REQUIRE(guard.count() == 0);
 */

// Thread-local allocation counter and tracking flag.
// Defined (with external linkage) in test_realtime_safety.cpp.
extern thread_local int  g_allocCount;
extern thread_local bool g_trackAllocs;

/**
 * RAII guard: enables allocation tracking on construction, disables on
 * destruction.  Resets the counter at construction time so only allocations
 * within the guarded scope are measured.
 */
struct AllocGuard
{
    AllocGuard()  { g_allocCount = 0; g_trackAllocs = true; }
    ~AllocGuard() { g_trackAllocs = false; }

    /** Return the number of heap allocations that occurred in this scope. */
    int count() const noexcept { return g_allocCount; }
};
