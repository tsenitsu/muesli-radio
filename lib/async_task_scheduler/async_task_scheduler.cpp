module async_task_scheduler;

namespace async_task_scheduler {

AsyncTaskScheduler::AsyncTaskScheduler(const unsigned int concurrencyLevel)
 :  m_executors(concurrencyLevel) {}

auto AsyncTaskScheduler::concurrencyLevel() const -> unsigned int {
    return static_cast<unsigned int>(m_executors.size());
}

auto AsyncTaskScheduler::enqueueTask(std::unique_ptr<AsyncTask> task, const unsigned int threadId) -> void {
    if (threadId >= m_executors.size())
        return;

    m_executors[threadId].enqueueTask(std::move(task));
}

auto makeAsyncTaskScheduler(const unsigned int concurrencyLevel) -> std::expected<std::unique_ptr<AsyncTaskScheduler>, std::string> {
    if (concurrencyLevel > 0)
        return std::make_unique<AsyncTaskScheduler>(concurrencyLevel);

    return std::unexpected { std::string { "Concurrency level must be greater than 0" } };
}

}