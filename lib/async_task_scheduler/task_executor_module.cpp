export module task_executor;

import std;

import async_task;

namespace async_task_scheduler {

export class TaskExecutor final {
public:
    TaskExecutor();
    ~TaskExecutor();

    auto enqueueTask(std::unique_ptr<AsyncTask> task) -> void;

protected:
    auto executeTasks(const std::stop_token& stopHandle) -> void;

    std::vector<std::unique_ptr<AsyncTask>> m_tasks;
    std::vector<std::unique_ptr<AsyncTask>> m_pendingTasks;

private:
    std::mutex m_taskAccess;
    std::condition_variable_any m_taskAvailable;
    std::jthread m_taskExecutor;
    std::stop_source m_executionHandle;
};

export auto makeTaskExecutor() -> std::unique_ptr<TaskExecutor>;

}