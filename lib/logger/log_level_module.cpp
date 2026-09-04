export module log_level;

import std;

namespace logger {

export enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

export [[nodiscard]] auto constexpr toString(const LogLevel level) -> std::string_view {
    switch (level) {
        case LogLevel::Debug:   return "D";
        case LogLevel::Info:    return "I";
        case LogLevel::Warning: return "W";
        case LogLevel::Error:   return "E";
        case LogLevel::Fatal:   return "F";
    }

    std::unreachable();
}

}
