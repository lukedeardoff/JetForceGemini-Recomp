#include "jfg/boot/executor.hpp"

#include <utility>
#include <algorithm>
#include <stdexcept>

namespace jfg::boot {

namespace {

// Thrown into a parked participant during shutdown to unwind its body so the
// OS thread can be joined cleanly. Never escapes participant_main.
struct ExecutorShutdown final {};

// The running participant's own id, so yield_to_scheduler() needs no argument.
thread_local int t_self = kSchedulerParticipant;

} // namespace

void BatonExecutor::enable_timing(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!participants_.empty()) throw std::logic_error("timing must be configured before spawn");
    timing_enabled_ = enabled;
}
BatonExecutor::Timing BatonExecutor::timing() {
    std::lock_guard<std::mutex> lock(mutex_);
    return timing_;
}
void BatonExecutor::publish_timing() {
    if (timing_enabled_) handoff_started_ = std::chrono::steady_clock::now();
}
void BatonExecutor::receive_timing() {
    if (!timing_enabled_) return;
    const auto elapsed = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - handoff_started_).count());
    ++timing_.wakes;
    timing_.wake_ns += elapsed;
    timing_.max_wake_ns = (std::max)(timing_.max_wake_ns, elapsed);
}

BatonExecutor::~BatonExecutor() {
    shutdown();
}

int BatonExecutor::spawn(Body body) {
    std::unique_lock<std::mutex> lock(mutex_);
    const int id = static_cast<int>(participants_.size());
    participants_.emplace_back();
    participants_[static_cast<std::size_t>(id)].started = true;
    // The new thread immediately parks on the baton (holder_ != id), so it
    // touches nothing until the scheduler resumes it.
    participants_[static_cast<std::size_t>(id)].thread =
        std::thread([this, id, body = std::move(body)]() {
            participant_main(id, body);
        });
    return id;
}

void BatonExecutor::participant_main(const int id, Body body) {
    t_self = id;
    wait_until_holder(id);
    try {
        if (shutting_down_) {
            throw ExecutorShutdown{};
        }
        body(*this, id);
    } catch (const ExecutorShutdown&) {
        // Cooperative cancellation during shutdown; fall through to hand back.
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        participants_[static_cast<std::size_t>(id)].finished = true;
        publish_timing();
        holder_ = kSchedulerParticipant;
    }
    cv_.notify_one();
}

bool BatonExecutor::resume(const int id) {
    hand_off(id);
    wait_until_holder(kSchedulerParticipant);
    std::unique_lock<std::mutex> lock(mutex_);
    return participants_[static_cast<std::size_t>(id)].finished;
}

void BatonExecutor::yield_to_scheduler() {
    const int self = t_self;
    hand_off(kSchedulerParticipant);
    wait_until_holder(self);
    if (shutting_down_) {
        throw ExecutorShutdown{};
    }
}

void BatonExecutor::hand_off(const int to) {
    std::condition_variable *ready = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        publish_timing();
        holder_ = to;
        ready = to == kSchedulerParticipant ? &cv_
            : participants_[static_cast<std::size_t>(to)].ready.get();
    }
    // Only the selected baton recipient can run. Waking the other parked
    // guest threads just makes them contend for this mutex and sleep again.
    ready->notify_one();
}

void BatonExecutor::wait_until_holder(const int self) {
    std::unique_lock<std::mutex> lock(mutex_);
    auto &ready = self == kSchedulerParticipant ? cv_
        : *participants_[static_cast<std::size_t>(self)].ready;
    ready.wait(lock, [&]() { return holder_ == self; });
    receive_timing();
}

void BatonExecutor::shutdown() {
    // Unwind each unfinished participant one at a time, in id order. Only the
    // baton holder ever runs, so this stays deterministic and race-free.
    for (std::size_t index = 0; index < participants_.size(); ++index) {
        bool finished = false;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            finished = participants_[index].finished;
            if (!finished) {
                shutting_down_ = true;
            }
        }
        if (!finished) {
            (void)resume(static_cast<int>(index));
        }
    }
    for (Participant& participant : participants_) {
        if (participant.thread.joinable()) {
            participant.thread.join();
        }
    }
}

} // namespace jfg::boot
