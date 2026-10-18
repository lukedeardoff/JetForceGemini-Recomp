// Phase 6 stackful executor tests. Verifies the baton discipline (one
// participant runs at a time), deterministic interleaving, run/yield/resume,
// and clean shutdown of a never-finishing (parked) participant.

#include "jfg/boot/executor.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace jfg::boot;

int failures = 0;

void check(const bool condition, const std::string_view message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

// Runs a two-participant cooperative scenario and returns the interleaving log.
// Because only the baton holder runs, the log is fully determined by the
// scheduler's resume order.
std::vector<std::string> run_scenario() {
    std::vector<std::string> log;
    BatonExecutor executor;

    // Participant A: log a0, yield, log a1, yield, finish.
    const int a = executor.spawn([&log](BatonExecutor& ex, int) {
        log.push_back("a0");
        ex.yield_to_scheduler();
        log.push_back("a1");
        ex.yield_to_scheduler();
        log.push_back("a2");
    });
    // Participant B: log b0, yield, log b1, finish.
    const int b = executor.spawn([&log](BatonExecutor& ex, int) {
        log.push_back("b0");
        ex.yield_to_scheduler();
        log.push_back("b1");
    });

    // Deterministic scheduler order: A, B, A, B, A.
    check(!executor.resume(a), "A not finished after first slice");
    check(!executor.resume(b), "B not finished after first slice");
    check(!executor.resume(a), "A not finished after second slice");
    check(executor.resume(b), "B finished after second slice");
    check(executor.resume(a), "A finished after third slice");
    return log;
}

void test_deterministic_interleaving() {
    const std::vector<std::string> first = run_scenario();
    const std::vector<std::string> second = run_scenario();
    const std::vector<std::string> expected = {
        "a0", "b0", "a1", "b1", "a2"};
    check(first == expected, "interleaving matches the scheduler order");
    check(first == second, "interleaving is deterministic across runs");
}

void test_single_runner_no_overlap() {
    // If two participants ever ran concurrently, this unguarded counter would
    // race; under the baton it cannot. We increment heavily while yielding.
    BatonExecutor executor;
    long counter = 0;
    const int a = executor.spawn([&counter](BatonExecutor& ex, int) {
        for (int i = 0; i < 1000; ++i) {
            ++counter;
            if ((i % 100) == 0) {
                ex.yield_to_scheduler();
            }
        }
    });
    const int b = executor.spawn([&counter](BatonExecutor& ex, int) {
        for (int i = 0; i < 1000; ++i) {
            ++counter;
            if ((i % 100) == 0) {
                ex.yield_to_scheduler();
            }
        }
    });
    bool a_done = false;
    bool b_done = false;
    // Drive both to completion, alternating.
    for (int round = 0; round < 100 && !(a_done && b_done); ++round) {
        if (!a_done) {
            a_done = executor.resume(a);
        }
        if (!b_done) {
            b_done = executor.resume(b);
        }
    }
    check(a_done && b_done, "both participants finished");
    check(counter == 2000, "no lost increments — exactly one ran at a time");
}

void test_shutdown_unwinds_parked_participant() {
    // A participant that yields forever must be unwound cleanly by shutdown
    // (no hang, thread joined), not leak or deadlock.
    bool reached_after_loop = false;
    {
        BatonExecutor executor;
        const int spinner = executor.spawn(
            [&reached_after_loop](BatonExecutor& ex, int) {
                while (true) {
                    ex.yield_to_scheduler();
                }
                // Unreachable; present to prove we do NOT fall through here.
                reached_after_loop = true;  // NOLINT
            });
        check(!executor.resume(spinner), "spinner yields, not finished");
        check(!executor.resume(spinner), "spinner still yielding");
        // executor destructor runs shutdown(): must unwind and join.
    }
    check(!reached_after_loop, "spinner body did not run past its wait loop");
    check(true, "shutdown completed without hanging");
}

void test_never_started_participant_shuts_down() {
    // A spawned-but-never-resumed participant must still join on shutdown.
    BatonExecutor executor;
    (void)executor.spawn([](BatonExecutor&, int) {
        // never resumed before shutdown
    });
    check(executor.participant_count() == 1U, "participant registered");
    // Destructor shutdown() must not hang joining the never-resumed thread.
}

void test_diagnostic_wake_boundaries() {
    for (bool enabled : {false, true}) {
        BatonExecutor ex;
        ex.enable_timing(enabled);
        const int id = ex.spawn([](BatonExecutor& body, int) {
            for (int i=0;i<20;++i) body.yield_to_scheduler();
        });
        for (int i=0;i<20;++i) check(!ex.resume(id), "timed body yielded");
        check(ex.resume(id), "timed body finished");
        const auto timing=ex.timing();
        check(timing.wakes==(enabled?42U:0U), "both sides of each transfer measured once");
        check(timing.max_wake_ns<=timing.wake_ns, "maximum is part of total");
        if (!enabled) check(timing.wake_ns==0 && timing.max_wake_ns==0, "ordinary transport stays unmeasured");
        bool rejected=false;
        try { ex.enable_timing(!enabled); } catch (const std::logic_error&) { rejected=true; }
        check(rejected, "no timing reconfiguration during active transport");
    }
}

} // namespace

int main() {
    test_diagnostic_wake_boundaries();
    test_deterministic_interleaving();
    test_single_runner_no_overlap();
    test_shutdown_unwinds_parked_participant();
    test_never_started_participant_shuts_down();
    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "all executor tests passed\n";
    return 0;
}
