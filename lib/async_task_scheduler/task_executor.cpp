module task_executor;

namespace async_task_scheduler {

TaskExecutor::TaskExecutor()
 :  m_tasks {},
    m_pendingTasks {},
    m_taskAccess {},
    m_taskAvailable {},
    m_taskExecutor { [this] (std::stop_token stopHandle) { executeTasks(stopHandle); } },
    m_executionHandle { m_taskExecutor.get_stop_source() } {}

TaskExecutor::~TaskExecutor() {
    m_executionHandle.request_stop();

    if (m_taskExecutor.joinable()) {
        m_taskExecutor.join();
    }
}


auto TaskExecutor::enqueueTask(std::unique_ptr<AsyncTask> task) -> void {
    {
        std::lock_guard lock(m_taskAccess);
        m_tasks.emplace_back(std::move(task));
    }

    m_taskAvailable.notify_one();
}

auto TaskExecutor::executeTasks(const std::stop_token& stopHandle) -> void {
    do {
        {
            std::unique_lock lock(m_taskAccess);

            if (m_tasks.empty()) {
                m_taskAvailable.wait(lock, stopHandle, [this, &stopHandle] () { return not m_tasks.empty() || stopHandle.stop_requested(); });
            }

            m_pendingTasks.swap(m_tasks);
        }

        for (auto& task : m_pendingTasks) {
            (*task)();
        }

        m_pendingTasks.erase(std::remove_if(std::ranges::begin(m_pendingTasks), std::ranges::end(m_pendingTasks),
            [] (const std::unique_ptr<AsyncTask>& task) { return task->done(); }), std::ranges::end(m_pendingTasks));
        {
            std::unique_lock lock(m_taskAccess);

            m_tasks.insert(std::ranges::end(m_tasks),
                std::make_move_iterator(std::ranges::begin(m_pendingTasks)),
                std::make_move_iterator(std::ranges::end(m_pendingTasks)));
        }

        m_pendingTasks.clear();
    } while (not stopHandle.stop_requested());
}

auto makeTaskExecutor() -> std::unique_ptr<TaskExecutor> {
    return std::make_unique<TaskExecutor>();
}

}