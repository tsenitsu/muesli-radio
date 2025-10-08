export module async_task;

import std;

import signal;
import dependency;

namespace async_task_scheduler {

export class AsyncTask {
public:
    AsyncTask() :  m_signal {}, m_dependencies {} {}
    AsyncTask(AsyncTask&&) = default;

    AsyncTask(const AsyncTask&) = delete;
    AsyncTask& operator=(const AsyncTask&) = delete;

    virtual ~AsyncTask() = default;

    virtual auto operator()() -> void = 0;
    [[nodiscard]] virtual auto done() const -> bool = 0;

    [[nodiscard]] auto dependency() const -> Dependency { return Dependency { m_signal }; }
    auto dependency(const AsyncTask& task) -> void { m_dependencies.push_back(task.dependency()); }

    [[nodiscard]] auto areDependenciesMet() const -> bool {
        for (const auto& dependency: m_dependencies) {
            if (not dependency.waitFor(std::chrono::milliseconds::zero()))
                return false;
        }

        return true;
    };

protected:
    auto signalCompletion() -> void { m_signal.signalCompletion(); }

    Signal m_signal;
    std::vector<Dependency> m_dependencies;
};

}