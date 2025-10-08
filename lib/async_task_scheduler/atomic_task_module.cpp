export module atomic_task;

import std;

import async_task;
import dependency;
import result;

namespace async_task_scheduler {

export template<typename Res, typename... ArgTypes>
class AtomicTask final : public AsyncTask {
public:
    template <typename Callable> requires std::is_invocable_r_v<Res, Callable, ArgTypes...>
    explicit AtomicTask(Callable&& callable, ArgTypes&&... args)
     :  m_task { std::forward<Callable>(callable) },
        m_args { std::forward<ArgTypes>(args)... },
        m_result { m_task.get_future().share() } {}

    auto operator()() -> void override {
        if (not areDependenciesMet())
            return;

        std::apply(m_task, m_args);
        signalCompletion();
    }

    [[nodiscard]] auto done() const -> bool override {
        return m_result.ready();
    }

    [[nodiscard]] auto result() const -> Result<Res> { return m_result; }

private:
    std::packaged_task<Res(ArgTypes...)> m_task;
    std::tuple<ArgTypes...> m_args;
    Result<Res> m_result;
};

export template<typename Callable, typename... ArgTypes>
AtomicTask(Callable&&, ArgTypes&&...) -> AtomicTask<std::invoke_result_t<Callable, ArgTypes...>, std::decay_t<ArgTypes>...>;

export template<typename Callable, typename... ArgTypes> requires std::invocable<Callable, ArgTypes...>
[[nodiscard]] auto makeAtomicTask(Callable&& callable, ArgTypes&&... args) -> auto {
    return std::make_unique<AtomicTask<std::invoke_result_t<Callable, ArgTypes...>, ArgTypes...>>(std::forward<Callable>(callable), std::forward<ArgTypes>(args)...);
}

}
