export module result;

import std;

namespace async_task_scheduler {

export template <typename Res>
class Result {
public:
    explicit Result(const std::shared_future<Res>& result) : m_result { result } {}

    [[nodiscard]] auto get() const -> auto { return m_result.get(); }
    [[nodiscard]] auto ready() const -> bool { return m_result.wait_for(std::chrono::milliseconds::zero()) == std::future_status::ready; }

private:
    std::shared_future<Res> m_result;
};

}