export module dependency;

import std;

import signal;

namespace async_task_scheduler {

export class Dependency final {
public:
    explicit Dependency(const Signal& signal) :  m_dependency { signal.m_result } {}

    auto wait() const -> void { m_dependency.wait(); }
    [[nodiscard]] auto waitFor(const std::chrono::milliseconds timeout) const -> bool { return m_dependency.wait_for(timeout) == std::future_status::ready; }

private:
    std::shared_future<void> m_dependency;
};

}