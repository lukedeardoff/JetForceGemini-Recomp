#pragma once

// Stack transport only: the executed guest OS chooses its next thread and
// restores the CPU context. No host priority policy or fabricated OS state.
#include "jfg/boot/executor.hpp"
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>

namespace jfg::boot {
template<class Context> class GuestThreadTransport final {
public:
  using Enter = std::function<void(std::uint32_t, Context&)>;
  using Repair = std::function<void(Context&)>;
  // The caller has already committed its CPU ERET state. Observe the validated
  // transfer before any other participant can run, not when this host call
  // eventually returns. This is a transport boundary, not a complete CPU model.
  struct EretBoundary {
    std::uint32_t from_thread, to_thread, target_pc;
  };
  using ObserveEret = std::function<void(const EretBoundary&, const Context&)>;
  using SchedulerWork = std::function<void()>;
  GuestThreadTransport(Enter enter, Repair repair, ObserveEret observe_eret = {},
                       SchedulerWork scheduler_work = {})
      : enter_(std::move(enter)), repair_(std::move(repair)),
        observe_eret_(std::move(observe_eret)),
        scheduler_work_(std::move(scheduler_work)) {}
  GuestThreadTransport(const GuestThreadTransport&) = delete;
  GuestThreadTransport& operator=(const GuestThreadTransport&) = delete;

  // Called before a guest yield call, or at interrupted PC before exception
  // entry. An ERET may resume only this observed continuation, never a lookup
  // into the middle of an arbitrary generated C function.
  void park_at(std::uint32_t pc) {
    require(running_, "continuation outside guest execution");
    threads_.at(current_).continuation = pc;
  }
  void eret(std::uint32_t guest_thread, std::uint32_t pc, Context& context) {
    require(running_ && guest_thread != 0, "invalid guest ERET owner");
    if (guest_thread == current_) {
      if (observe_eret_) observe_eret_({current_, guest_thread, pc}, context);
      return;
    }
    require(current_ == 0 || threads_.at(current_).continuation.has_value(), "missing parked continuation");
    // Reject a known target's wrong continuation before publishing a boundary
    // or yielding. Retain the scheduler/resume checks as independent guards.
    const auto destination = threads_.find(guest_thread);
    require(destination == threads_.end() || destination->second.continuation == pc,
            "unobserved guest resume PC");
    if (observe_eret_) observe_eret_({current_, guest_thread, pc}, context);
    selected_ = guest_thread;
    entry_pc_ = pc;
    transfer_ = context;
    executor_.yield_to_scheduler();
    require(selected_ == current_ && threads_.at(current_).continuation == entry_pc_,
            "guest ERET continuation mismatch");
    context = transfer_;
    repair_(context);
  }
  // Bounded diagnostic completion; leaves the current host continuation
  // parked so shutdown can unwind it without executing more guest work.
  void stop() {
    require(running_, "stop outside guest execution");
    stopped_ = true;
    executor_.yield_to_scheduler();
    throw std::logic_error("stopped guest resumed");
  }
  void run(std::uint32_t entry_pc, const Context& initial, std::uint64_t handoff_limit) {
    require(!started_ && handoff_limit != 0, "invalid transport start");
    started_ = true;
    selected_ = 0; // bootstrap is not an OSThread
    transfer_ = initial;
    entry_pc_ = entry_pc;
    while (!stopped_) {
      require(handoffs_++ < handoff_limit, "guest handoff limit");
      auto [it, inserted] = threads_.try_emplace(selected_);
      if (inserted) {
        it->second.participant = executor_.spawn([this](auto&, int) {
          Context context = transfer_;
          repair_(context);
          enter_(entry_pc_, context);
          // A returning thread requires the guest cleanup routine; silently
          // terminating it would leave the original run queue inconsistent.
          throw std::logic_error("guest thread returned without cleanup");
        });
      } else {
        require(it->second.continuation == entry_pc_, "unobserved guest resume PC");
      }
      // Every participant is parked here; host work runs on the caller's
      // thread without changing guest scheduling or exposing concurrent state.
      if (scheduler_work_) scheduler_work_();
      current_ = selected_;
      running_ = true;
      const bool finished = executor_.resume(it->second.participant);
      running_ = false;
      require(!finished, "guest participant exited early");
    }
    executor_.shutdown();
  }
  [[nodiscard]] std::uint32_t current() const { return current_; }
  [[nodiscard]] std::uint64_t handoffs() const { return handoffs_; }
  void enable_timing(bool enabled) { executor_.enable_timing(enabled); }
  [[nodiscard]] BatonExecutor::Timing timing() { return executor_.timing(); }
private:
  static void require(bool valid, const char* reason) {
    if (!valid) throw std::logic_error(reason);
  }
  struct Thread { int participant = -1; std::optional<std::uint32_t> continuation; };
  // Executor is destroyed first, while callbacks and state are still alive.
  Enter enter_;
  Repair repair_;
  ObserveEret observe_eret_;
  SchedulerWork scheduler_work_;
  std::map<std::uint32_t, Thread> threads_;
  Context transfer_{};
  std::uint32_t current_ = 0, selected_ = 0, entry_pc_ = 0;
  std::uint64_t handoffs_ = 0;
  bool started_ = false, running_ = false, stopped_ = false;
  BatonExecutor executor_;
};
} // namespace jfg::boot
