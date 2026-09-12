export module logger_manager;

import std;

import logger;
import task_manager;
import async_task_scheduler;

namespace ats = async_task_scheduler;

namespace managers {

export class LoggerManager final: public ats::TaskManager {
public:
    explicit LoggerManager(ats::AsyncTaskScheduler& scheduler);
    ~LoggerManager() override;

    auto enqueueLogEntry(std::optional<logger::LogEntry> entry) -> void;
    auto flushLogs() -> void;

    [[nodiscard]] auto isLoggingEnabled() const -> bool;

private:
    std::unique_ptr<logger::Logger> m_logger;
    std::mutex m_logEntriesMutex;
    std::vector<logger::LogEntry> m_logEntries;
    std::atomic_bool m_loggingEnabled;
};

export [[nodiscard]] auto makeLoggerManager(ats::AsyncTaskScheduler& scheduler) -> std::expected<std::unique_ptr<LoggerManager>, std::string>;

}