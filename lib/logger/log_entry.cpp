module log_entry;

import utils;

namespace logger {

LogEntry::LogEntry(const LogLevel level, std::string_view entity,
        std::string_view message, std::chrono::time_point<std::chrono::system_clock> timestamp)
         :  m_level { level }, m_entity { entity }, m_message { message }, m_timestamp { timestamp } {}

auto toString(const LogEntry& logEntry) -> std::string {
    return std::format("{} | {} | {} | {}", utils::systemClockToString(logEntry.m_timestamp, " "), toString(logEntry.m_level), logEntry.m_entity, logEntry.m_message);
}

auto makeLogEntry(LogLevel level, std::string_view entity, std::string_view message, std::chrono::time_point<std::chrono::system_clock> timestamp) -> std::optional<LogEntry> {
    constexpr auto minLogLevel {
#ifndef NDEBUG
        LogLevel::Debug
#else
        LogLevel::Info
#endif
    };

    if (static_cast<std::underlying_type_t<LogLevel>>(level) < static_cast<std::underlying_type_t<LogLevel>>(minLogLevel))
        return std::nullopt;

    return std::make_optional<LogEntry>(level, entity, message, timestamp);
}

}

