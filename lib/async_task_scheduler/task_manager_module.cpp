export module task_manager;

import std;

import async_task;
import async_task_scheduler;

namespace async_task_scheduler {

export class TaskManager {
public:
    template <typename... Task> requires (std::is_convertible_v<Task, std::unique_ptr<AsyncTask>> && ...) and (sizeof...(Task) > 0)
    auto enqueueTasks(Task&&... tasks) -> void {
        thread_local std::mt19937 engine { std::random_device{}() };
        std::uniform_int_distribution<unsigned int> distribution { 0, m_scheduler.concurrencyLevel() - 1 };

        std::vector<Dependency> taskDependencies {};
        taskDependencies.reserve(sizeof...(Task));
        (taskDependencies.emplace_back(tasks->dependency()), ...); // First capture dependencies

        (m_scheduler.enqueueTask(std::forward<Task>(tasks), distribution(engine)), ...); // Then move the tasks

        {
            std::lock_guard lock { m_pendingTasksMutex };
            m_pendingTasks.insert(std::ranges::end(m_pendingTasks), std::make_move_iterator(std::ranges::begin(taskDependencies)), std::make_move_iterator(std::ranges::end(taskDependencies)));
            std::erase_if(m_pendingTasks, [](const auto& dependency) { return dependency.waitFor(std::chrono::milliseconds(0)); });
        }
    }

    explicit TaskManager(AsyncTaskScheduler& scheduler)
     :  m_scheduler { scheduler } {}

    virtual ~TaskManager() = default;

protected:
    auto waitForAllTasks() -> void {
        std::vector<Dependency> tasksToWait {};
        {
            std::lock_guard lock { m_pendingTasksMutex };
            tasksToWait.swap(m_pendingTasks);
        }

        for (const auto& dependency : tasksToWait)
            dependency.wait();
    }

private:
    AsyncTaskScheduler& m_scheduler;
    std::mutex m_pendingTasksMutex;
    std::vector<Dependency> m_pendingTasks;
};

export [[nodiscard]] auto makeTaskManager(AsyncTaskScheduler& scheduler) -> std::unique_ptr<TaskManager> {
    return std::make_unique<TaskManager>(scheduler);
}

}