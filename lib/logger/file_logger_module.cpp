export module file_logger;

import std;

import logger;

namespace logger {

export class FileLogger final : public Logger {
public:
    FileLogger(std::filesystem::path filePath,
               std::uintmax_t maxFileSize,
               std::chrono::milliseconds retentionPeriod);

    auto log(std::span<const LogEntry> entries) -> void override;

private:
    [[nodiscard]] auto openFile() -> bool;
    auto closeFile() -> void;
    auto rotateFile() const -> void;
    auto deleteOldFiles() const -> void;

    std::filesystem::path m_filePath;
    std::uintmax_t m_maxFileSize;
    std::ofstream m_file;
    std::chrono::milliseconds m_retentionPeriod;
};

export [[nodiscard]] auto makeFileLogger(std::filesystem::path filePath, std::uintmax_t maxFileSize, std::chrono::milliseconds retentionPeriod)-> std::expected<std::unique_ptr<Logger>, std::string>;

}