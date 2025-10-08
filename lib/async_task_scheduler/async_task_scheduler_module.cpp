export module async_task_scheduler;

export import task_executor;
export import async_task;
export import atomic_task;
export import dependency;
export import signal;
export import result;
export import resumable_task;

import std;

namespace async_task_scheduler {

export class AsyncTaskScheduler final {
public:
    explicit AsyncTaskScheduler(unsigned int concurrencyLevel);

    [[nodiscard]] auto concurrencyLevel() const -> unsigned int;
    auto enqueueTask(std::unique_ptr<AsyncTask> task, unsigned int threadId = 0) -> void;

private:
    std::vector<TaskExecutor> m_executors;
};

export auto makeAsyncTaskScheduler(unsigned int concurrencyLevel) -> std::expected<std::unique_ptr<AsyncTaskScheduler>, std::string>;

}