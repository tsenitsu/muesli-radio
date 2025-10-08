export module resumable_task;

import std;

import async_task;
import result;

namespace async_task_scheduler {

template <typename T>
struct promise_type;

export template <typename Res>
class ResumableTask final: public AsyncTask {
public:
    using promise_type = promise_type<Res>;

    explicit ResumableTask(std::coroutine_handle<promise_type> handle): m_handle { handle } {}
    ResumableTask(ResumableTask&& other) noexcept :  m_handle { std::move(other.m_handle) } {}

    ResumableTask(const ResumableTask& other) = delete;
    ResumableTask& operator=(ResumableTask&& other) = delete;

    [[nodiscard]] auto done() const -> bool override { return m_handle.done();};

    auto operator()() -> void override {
        if (not areDependenciesMet())
            return;

        if (not m_handle.done())
            m_handle.resume();

        if (m_handle.done()) {
            signalCompletion();
        }
    }

    [[nodiscard]] auto result() const -> Result<Res> { return m_handle.promise().m_result; };

private:
    std::coroutine_handle<promise_type> m_handle;

};

template <typename Res>
struct promise_type {
    promise_type() :  m_promise {}, m_result { m_promise.get_future().share() } {}

    auto get_return_object() noexcept -> ResumableTask<Res> { return ResumableTask<Res>{ std::coroutine_handle<promise_type>::from_promise(*this)}; }

    static auto initial_suspend() noexcept -> std::suspend_always { return {}; }
    static auto final_suspend() noexcept -> std::suspend_always { return {}; }

    static auto unhandled_exception() noexcept -> void {}

    auto return_value(Res&& value) -> void { m_promise.set_value(std::forward<Res>(value));  }

    std::promise<Res> m_promise;
    Result<Res> m_result;
};

template <>
struct promise_type<void> {
    promise_type() :  m_promise {}, m_result { m_promise.get_future().share() } {}

    auto get_return_object() noexcept -> ResumableTask<void> { return ResumableTask { std::coroutine_handle<promise_type>::from_promise(*this)}; }
    static auto initial_suspend() noexcept -> std::suspend_always { return {}; }
    static auto final_suspend() noexcept -> std::suspend_always { return {}; }

    static auto unhandled_exception() noexcept -> void {}

    auto return_void() -> void { m_promise.set_value();  }

    std::promise<void> m_promise;
    Result<void> m_result;
};

export template<typename Res>
[[nodiscard]] auto makeResumableTask(ResumableTask<Res>&& task) -> auto {
    return std::make_unique<ResumableTask<Res>>(std::forward<ResumableTask<Res>>(task));
}

}
