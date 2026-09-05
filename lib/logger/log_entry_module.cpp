export module log_entry;

import std;

import log_level;

namespace logger {

export class LogEntry final {
public:
    LogEntry(LogLevel level, std::string_view entity, std::string_view message, std::chrono::time_point<std::chrono::system_clock> timestamp = std::chrono::system_clock::now());

    LogLevel m_level;
    std::string m_entity;
    std::string m_message;
    std::chrono::time_point<std::chrono::system_clock> m_timestamp;
};

export [[nodiscard]] auto toString(const LogEntry& logEntry) -> std::string;
export [[nodiscard]] auto makeLogEntry(LogLevel level, std::string_view entity, std::string_view message, std::chrono::time_point<std::chrono::system_clock> timestamp = std::chrono::system_clock::now()) -> std::optional<LogEntry>;

}