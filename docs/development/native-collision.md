# Native collision traversal

The experimental Windows preview has a bounded native implementation of the collision BSP traversal used by drone behavior. Detailed profiling identified repeated generated instruction bookkeeping and calls inside this traversal. The implementation retains the qualified original arithmetic, register/stack effects and scratch-memory ordering, while omitting instruction-by-instruction accounting during eligible native elapsed-time execution.

This improves one CPU component. It does not guarantee that every level, loading transition, damage event or host-load condition will maintain 30 game updates per second. Current measurements and exact binary identities are in [qualification](../../evidence/high-refresh-native-collision.json); remaining work is tracked in [PERF-001](issues/goldwood-combat-slowdown.md).

## Compatibility and bounds

JFG_EXPERIMENTAL_NATIVE_COLLISION is off by default. It requires MSVC on Windows, the private live runtime and native timing preview. The build derives three bodies from owner-generated private code; exact normalized source digests, span counts and the closed call set are checked. A different generator output is rejected until separately qualified. Generated instructions remain private and are never added to the repository.

The dispatcher requires the expected overlay, an eligible native-clock certificate, matching Status/I/O revision and no recorded-clock or instruction observer. Original timing and recorded prefixes retain generated execution. With the feature compiled, normal experimental Play enables it; JFG_DIAGNOSTIC_BSP_NATIVE=0 disables it for developer comparisons, and 1 requests it explicitly. Requesting 1 from a build without the feature fails rather than silently implying it is active.

Each top-level entry admits at most 32 native nodes and 128 edges, at most 16 edges per leaf, and at most 24 native recursion levels. It checks addresses, stack shape, floating-register mode and bounded finite values. A 50-microsecond deadline is checked before each node. That is an admission limit, not a hard maximum call time: OS scheduling and original fallback can take longer.

An unsupported or exhausted node delegates to the original function at that node, preserving work already performed by its parent. The original subtree cannot re-enter the native helper. Scope guards restore budgets, active-call state and fallback state during cancellation. If any node used native execution, the dispatcher invalidates its clock certificate so the next generated instruction refreshes elapsed clocks and events. Native instruction totals intentionally differ from Original mode; they are not equivalent workload measures across those modes.

## Validation

From an x64 MSVC developer shell, use a new private output directory:

    python scripts/validate_native_collision.py --generated-root D:/private/generated --output D:/private/collision-check

The fixture compiles the actual production module against the generated original C. It reserves the same synthetic guest-address layout, compares 50,000 varied trees and mixed native/original budgets, and checks complete CPU state, stack, scratch globals, floating environment and periodic complete physical RAM. It separately exercises 18 rejected admission states and exception cleanup. An earlier live shadow capture compares 256 sampled game calls. Its first rejection was a test-range mistake above the helper's stack write footprint; that failed evidence is retained in PERF-001.

Exact recorded combat and long Original-mode comparisons check RAM, saves, instruction observations and interpolation decisions. Native 120/360 comparisons use a common verified recorded prefix before returning to real-time operation; their subsequent AI/navigation and scheduling can diverge. A normal Play capture separately checks that the default path enables the helper, consumes the saved route and finishes without reported unsupported access or audio-buffer errors. Hidden captures do not establish owner visual acceptance or physical display pacing.

## Diagnostics

With an existing gameplay trace, bsp-execution emits VI, controller poll, top-level calls, native nodes, native edges, fallback nodes, expired-budget/deadline/depth rejections, invalid-or-insufficient-edge-budget rejections, maximum top-level call nanoseconds, and clock-refresh requests. These are per-root-update differences on the owning host thread. The maximum includes original fallback and host preemption; it is not a promise of a 50-microsecond completion time.

Full function profiling measures the compiled closure under its BSP entry; direct internal calls do not create additional guest-dispatch rows. Use the native node/edge counters to interpret that difference. Ordinary FPS logging leaves expensive function/submission profiling off. [Profiler controls and cadence reporting](function-profiling.md) describe separate game-update gaps, present-return stalls and transition markers.
