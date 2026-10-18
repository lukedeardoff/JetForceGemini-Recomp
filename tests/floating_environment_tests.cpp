#include "jfg/renderer/floating_environment.hpp"
#include <cstdlib>
#include <future>
#if defined(_MSC_VER)
#pragma fenv_access(on)
#endif
static void require(bool value) { if (!value) std::abort(); }
int main() {
    std::fenv_t original{};
    require(jfg::capture_floating_environment(original));
    require(std::fesetround(FE_DOWNWARD) == 0);
    require(std::feclearexcept(FE_ALL_EXCEPT) == 0);
    require(std::feraiseexcept(FE_INEXACT) == 0);
    std::fenv_t incoming{};
    require(jfg::capture_floating_environment(incoming));
    for (int iteration = 0; iteration < 16; ++iteration) {
        const bool fail = iteration % 2 != 0;
        std::async(std::launch::async, [incoming, fail] {
            std::fenv_t pool_original{};
            require(jfg::capture_floating_environment(pool_original));
            require(std::fesetround(FE_UPWARD) == 0);
            require(std::feclearexcept(FE_ALL_EXCEPT) == 0);
            require(std::feraiseexcept(FE_DIVBYZERO) == 0);
            try {
                const jfg::ScopedFloatingEnvironment environment(&incoming);
                require(std::fegetround() == FE_DOWNWARD);
                require((jfg::floating_exception_flags() & FE_INEXACT) != 0);
                require((jfg::floating_exception_flags() & FE_DIVBYZERO) == 0);
                require(std::feraiseexcept(FE_OVERFLOW) == 0);
                if (fail) throw 17;
            } catch (int value) { require(fail && value == 17); }
            require(std::fegetround() == FE_UPWARD);
            require((jfg::floating_exception_flags() & FE_DIVBYZERO) != 0);
            require((jfg::floating_exception_flags() & (FE_INEXACT | FE_OVERFLOW)) == 0);
            require(std::fesetenv(&pool_original) == 0);
        }).get();
        require(std::fegetround() == FE_DOWNWARD);
        require((jfg::floating_exception_flags() & FE_INEXACT) != 0);
        require((jfg::floating_exception_flags() & (FE_DIVBYZERO | FE_OVERFLOW)) == 0);
    }
    {
        const jfg::ScopedFloatingEnvironment disabled(nullptr);
        require(std::fesetround(FE_TOWARDZERO) == 0);
    }
    require(std::fegetround() == FE_TOWARDZERO);
    jfg::merge_floating_exceptions(FE_DIVBYZERO);
    require((jfg::floating_exception_flags() & (FE_INEXACT | FE_DIVBYZERO)) == (FE_INEXACT | FE_DIVBYZERO));
    require(std::fesetenv(&original) == 0);
}
