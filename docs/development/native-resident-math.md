# Native resident math dispatch

The experimental native preview uses bounded implementations of Arctanf, mathDiffAngle, Sinf and Cosf. The detailed function profiler identified these frequent calls in movement, collision and pathfinding. The replacement avoids the generic generated-function wrapper and repeated instruction dispatch while retaining the original quantized guest lookup tables.

This is a measured component optimization, not completion of the no-20-FPS objective. Four alternating exact recorded combat comparisons measured control medians of 26.564/25.458 ms and optimized medians of 25.498/23.970 ms. The source-integrated confirmation measured 26.786 ms disabled and 25.682 ms enabled. Native repetitions still contain occasional three-VI updates.

## Admission and compatibility

The helpers reserve their entire bounded original path within the existing native clock quantum. Angle uses a maximum of 37 operations, difference 10, and sine/cosine 20. They preserve complete tested register results, instruction/count metadata, floating exceptions and guest RAM. They cannot call another guest function, yield, or write guest memory. Clock polling and interrupt/device ownership remain in the existing checked handler.

Original timing, insufficient poll budget, changed Status/I/O revision, unsupported floating-register layout, nonfinite angle inputs/table values, and explicit instruction/focused observations retain generated execution. The dispatcher preserves MMIO failure handling, dispatch history and generated-call counts. Include native_resident_math.h after the selected timing header; including both generated and source timing definitions is unsupported.

JFG_DIAGNOSTIC_RESIDENT_MATH=0 disables these helpers for a developer comparison. Value 1 explicitly enables them in an eligible native profile. With no variable, only the experimental native preview enables them. Gameplay tracing records resident-math rows containing VI, controller poll and per-update angle/difference/trig call counts.

## Reproduce the reference fixture

Use an x64 MSVC developer environment and a locally generated root containing the four expected resident functions. Outputs contain generated C, so choose a private directory and never add them to source control:

    python scripts/validate_native_resident_math.py --generated-root D:/private/generated --output D:/private/resident-math-check

The original synthetic fixture runs 100,000 cases for each helper against generated C using matched precise/C11 reference options and the runtime's C++ options. It varies register state, floating rounding modes, native admission, poll budgets and boundary values, including the 16-bit sine/cosine domain. It compares the whole CPU context, timing state, Count publication, callbacks, floating exceptions and unchanged RAM. Only batching diagnostic totals are excluded from equality.

The 400,000-case fixture, recorded RAM/save/instruction/interpolation comparisons, retained 17,000-VI Original comparison and native 120/360 observations are separately recorded in [qualification](../../evidence/high-refresh-resident-math.json). The bounded [function profiler](function-profiling.md) remains disabled during ordinary play. Full instrumented FPS is not performance acceptance.

The transferable lesson is to measure frequent wrapper costs as well as arithmetic. A previous angle-body-only replacement did not establish a consistent gain; avoiding eligible generic dispatch work produced a repeatable component saving. Larger collision/pathfinding and graphics-submission costs remain targets in [PERF-001](issues/goldwood-combat-slowdown.md).
