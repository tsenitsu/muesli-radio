module console_logger;

namespace logger {

auto ConsoleLogger::log(const std::vector<std::unique_ptr<LogEntry>>& entries) -> void {
    for (auto& logEntry : entries) {
        std::println("{}", toString(*logEntry));
    }
}

auto makeConsoleLogger() -> std::unique_ptr<Logger> {
    return std::make_unique<ConsoleLogger>();
}

}