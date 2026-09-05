export module logger;

import std;

export import log_entry;
export import log_level;

namespace logger {

export class Logger {
public:
    virtual ~Logger() = default;

    virtual auto log(std::span<const LogEntry> entries) -> void = 0;
};

}