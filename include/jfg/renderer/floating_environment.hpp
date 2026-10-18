#pragma once
#include <cfenv>

namespace jfg {
// A worker may be reused by a thread pool. Restore its prior environment on
// both normal return and unwinding; never leave the caller's settings behind.
class ScopedFloatingEnvironment final {
public:
    explicit ScopedFloatingEnvironment(const std::fenv_t* incoming);
    ~ScopedFloatingEnvironment() noexcept;
    ScopedFloatingEnvironment(const ScopedFloatingEnvironment&) = delete;
    ScopedFloatingEnvironment& operator=(const ScopedFloatingEnvironment&) = delete;
private:
    std::fenv_t previous_{};
    bool active_ = false;
};
bool capture_floating_environment(std::fenv_t& environment) noexcept;
int floating_exception_flags() noexcept;
void merge_floating_exceptions(int flags);
}
