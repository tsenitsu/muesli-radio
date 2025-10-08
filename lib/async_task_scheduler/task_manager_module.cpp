export module task_manager;

import std;

import async_task;
import async_task_scheduler;

namespace async_task_scheduler {

class TaskManager final {
public:
    template <typename... Task> requires (std::is_convertible_v<Task, std::unique_ptr<AsyncTask>> && ...) and (sizeof...(Task) > 0)
    auto enqueueTasks(Task&&... tasks) -> void {

        if constexpr (sizeof...(Task) == 1) {
            m_scheduler.enqueueTask(std::move(tasks...), m_threadId);
            updateThreadId();
        } else {
            std::vector<std::unique_ptr<AsyncTask>> taskVector {};
            taskVector.reserve(sizeof...(tasks));
            (taskVector.emplace_back(std::forward<Task>(tasks)), ...);

            for (auto& task: taskVector) {
                m_scheduler.enqueueTask(std::move(task), m_threadId);
                updateThreadId();
            }
        }
    }

    explicit TaskManager(AsyncTaskScheduler& scheduler)
     :  m_scheduler { scheduler },
        m_concurrencyLevel { m_scheduler.concurrencyLevel() },
        m_threadId { 0 } {}

private:
    auto updateThreadId() -> void { m_threadId = ++m_threadId % m_scheduler.concurrencyLevel(); }

    AsyncTaskScheduler& m_scheduler;
    unsigned int m_concurrencyLevel;
    unsigned int m_threadId;
};

export [[nodiscard]] auto makeTaskManager(AsyncTaskScheduler& scheduler) -> std::unique_ptr<TaskManager> {
    return std::make_unique<TaskManager>(scheduler);
}

}