export module task_manager;

import std;

import async_task;
import async_task_scheduler;

namespace async_task_scheduler {

export class TaskManager {
public:
    // Random thread per task
    template <typename... Task> requires (std::is_convertible_v<Task, std::unique_ptr<AsyncTask>> && ...) and (sizeof...(Task) > 0)
    auto enqueueTasks(Task&&... tasks) -> void {
        std::uniform_int_distribution<unsigned int> distribution { 0, m_scheduler.concurrencyLevel() - 1 };

        enqueue([&] { return distribution(m_randomEngine); }, std::forward<Task>(tasks)...);
    }

    // All tasks on the given thread; falls back to random selection if out of range
    template <typename... Task> requires (std::is_convertible_v<Task, std::unique_ptr<AsyncTask>> && ...) and (sizeof...(Task) > 0)
    auto enqueueTasks(unsigned int threadId, Task&&... tasks) -> void {
        if (threadId >= m_scheduler.concurrencyLevel()) {
            enqueueTasks(std::forward<Task>(tasks)...);
            return;
        }

        enqueue([threadId] { return threadId; }, std::forward<Task>(tasks)...);
    }

    explicit TaskManager(AsyncTaskScheduler& scheduler)
     :  m_scheduler { scheduler },
        m_pendingTasksMutex {},
        m_pendingTasks {} {}


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
    template <typename PickThread, typename... Task>
    auto enqueue(PickThread pickThread, Task&&... tasks) -> void {
        std::vector<Dependency> taskDependencies {};
        taskDependencies.reserve(sizeof...(Task));
        (taskDependencies.emplace_back(tasks->dependency()), ...); // First capture dependencies

        (m_scheduler.enqueueTask(std::forward<Task>(tasks), pickThread()), ...); // Then move the tasks

        {
            std::lock_guard lock { m_pendingTasksMutex };
            m_pendingTasks.insert(std::ranges::end(m_pendingTasks), std::make_move_iterator(std::ranges::begin(taskDependencies)), std::make_move_iterator(std::ranges::end(taskDependencies)));
            std::erase_if(m_pendingTasks, [](const auto& dependency) { return dependency.waitFor(std::chrono::milliseconds(0)); });
        }
    }

    static inline thread_local std::mt19937 m_randomEngine { std::random_device{}() };

    AsyncTaskScheduler& m_scheduler;
    std::mutex m_pendingTasksMutex;
    std::vector<Dependency> m_pendingTasks;
};

export [[nodiscard]] auto makeTaskManager(AsyncTaskScheduler& scheduler) -> std::unique_ptr<TaskManager> {
    return std::make_unique<TaskManager>(scheduler);
}

}