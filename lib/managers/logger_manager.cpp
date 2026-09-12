module logger_manager;

#ifndef NDEBUG
import console_logger;
#else
import file_logger;
#endif

namespace ats = async_task_scheduler;

namespace managers {

ats::ResumableTask<void> writeLog(std::chrono::milliseconds triggerInterval, LoggerManager& loggerManager) {
    auto triggerTime { std::chrono::steady_clock::now() + triggerInterval };

    while (loggerManager.isLoggingEnabled()) {
        while (loggerManager.isLoggingEnabled() &&
            std::chrono::steady_clock::now() < triggerTime)
            co_await std::suspend_always {};

        loggerManager.flushLogs();

        triggerTime += triggerInterval;
        co_await std::suspend_always {};
    }
}

LoggerManager::LoggerManager(ats::AsyncTaskScheduler &scheduler)
 :  TaskManager { scheduler },
    m_logger { nullptr },
    m_logEntriesMutex {},
    m_logEntries {},
    m_loggingEnabled { true } {
#ifndef NDEBUG
    m_logger = logger::makeConsoleLogger();
#else
    constexpr std::uintmax_t logFileSize { 5'000'000 /* in bytes */ };

    std::filesystem::path logFilePath { std::filesystem::current_path() /
        std::filesystem::path { "log" } /
        std::filesystem::path { "muesli_radio.log" } };

    if (auto loggerResult { logger::makeFileLogger(std::move(logFilePath), logFileSize) }; not loggerResult.has_value()) {
        throw std::runtime_error { std::string { std::format("Error creating file logger: {}", loggerResult.error()) } };
    } else {
        m_logger.swap(loggerResult.value());
    }
#endif

    std::unique_ptr<ats::AsyncTask> writeTask { ats::makeResumableTask<void>(writeLog(std::chrono::milliseconds { 500 }, *this)) };
    enqueueTasks(std::move(writeTask));
}

LoggerManager::~LoggerManager() {
    m_loggingEnabled.store(false, std::memory_order_release);
    waitForAllTasks();
    flushLogs();
}

auto LoggerManager::enqueueLogEntry(std::optional<logger::LogEntry> entry) -> void {
    if (not entry.has_value())
        return;

    std::lock_guard lock { m_logEntriesMutex };
    m_logEntries.emplace_back(std::move(*entry));
}

auto LoggerManager::flushLogs() -> void {
    std::vector<logger::LogEntry> logEntriesToFlush {};
    {
        std::lock_guard lock { m_logEntriesMutex };
        logEntriesToFlush.swap(m_logEntries);
    }

    if (logEntriesToFlush.empty())
        return;

    std::ranges::sort(logEntriesToFlush, [] (const auto& first, const auto& second) { return first.m_timestamp < second.m_timestamp; });
    m_logger->log(logEntriesToFlush);
}

auto LoggerManager::isLoggingEnabled() const -> bool {
    return m_loggingEnabled.load(std::memory_order_acquire);
}

auto makeLoggerManager(ats::AsyncTaskScheduler& scheduler) -> std::expected<std::unique_ptr<LoggerManager>, std::string> {
    try {
        return std::make_unique<LoggerManager>(scheduler);
    } catch (const std::exception& e) {
        return std::unexpected { std::string { e.what() } };
    }
}

}
