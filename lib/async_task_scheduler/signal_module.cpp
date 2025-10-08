export module signal;

import std;

namespace async_task_scheduler {

export class Signal final {
public:
    Signal() :  m_signal {}, m_result { m_signal.get_future() } {}

    auto signalCompletion() -> void { m_signal.set_value(); }

private:
    std::promise<void> m_signal;
    std::shared_future<void> m_result;

    friend class Dependency;
};

}

