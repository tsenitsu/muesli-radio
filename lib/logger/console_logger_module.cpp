export module console_logger;

import std;

import logger;

namespace logger {

export class ConsoleLogger final : public Logger {
public:
    auto log(std::span<const LogEntry> entries) -> void override;
};

export [[nodiscard]] auto makeConsoleLogger() -> std::unique_ptr<Logger>;

}