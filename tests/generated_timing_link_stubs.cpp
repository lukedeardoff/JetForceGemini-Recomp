#include <cstdlib>

// The link smoke test retains generated bodies but must not execute them.
// Original-timing roots need these exports too. Fail closed if a future smoke
// registry accidentally enters an instruction hook without a real runtime.
struct JfgOriginalTimingFast;
extern "C" {
JfgOriginalTimingFast* jfg_original_timing_state = nullptr;
void jfg_phase9_execution_probe_predecoded(unsigned, unsigned, unsigned, void*) {
    std::abort();
}
}

