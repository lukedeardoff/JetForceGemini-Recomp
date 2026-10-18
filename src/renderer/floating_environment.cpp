#include "jfg/renderer/floating_environment.hpp"
#include <exception>
#include <stdexcept>
#if defined(_MSC_VER)
#pragma fenv_access(on)
#endif

namespace jfg {
bool capture_floating_environment(std::fenv_t& environment) noexcept {
    return std::fegetenv(&environment) == 0;
}
int floating_exception_flags() noexcept {
    return std::fetestexcept(FE_ALL_EXCEPT);
}
void merge_floating_exceptions(int flags) {
    if (flags != 0 && std::feraiseexcept(flags) != 0)
        throw std::runtime_error("Cannot merge renderer floating-point flags");
}
ScopedFloatingEnvironment::ScopedFloatingEnvironment(const std::fenv_t* incoming) {
    if (incoming == nullptr) return;
    if (std::fegetenv(&previous_) != 0)
        throw std::runtime_error("Cannot capture worker floating-point environment");
    if (std::fesetenv(incoming) != 0) {
        if (std::fesetenv(&previous_) != 0) std::terminate();
        throw std::runtime_error("Cannot set worker floating-point environment");
    }
    active_ = true;
}
ScopedFloatingEnvironment::~ScopedFloatingEnvironment() noexcept {
    if (active_ && std::fesetenv(&previous_) != 0) std::terminate();
}
}
