export module file_logger;

import std;

import logger;

namespace logger {

export class FileLogger final : public Logger {
public:
    FileLogger(std::filesystem::path filePath, std::uintmax_t maxFileSize);

    auto log(std::span<const LogEntry> entries) -> void override;

private:
    [[nodiscard]] auto openFile() -> bool;
                  auto closeFile() -> void;
                  auto rotateFile() -> void;

    std::filesystem::path m_filePath;
    std::uintmax_t m_maxFileSize;
    std::ofstream m_file;
};

export [[nodiscard]] auto makeFileLogger(std::filesystem::path filePath, std::uintmax_t maxFileSize) -> std::expected<std::unique_ptr<Logger>, std::string>;

}
