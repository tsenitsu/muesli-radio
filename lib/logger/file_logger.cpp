module file_logger;

import utils;

namespace logger {

FileLogger::FileLogger(std::filesystem::path filePath, const std::uintmax_t maxFileSize)
 :  m_filePath { std::move(filePath) },
    m_maxFileSize { maxFileSize },
    m_file {} {

    if (m_filePath.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(m_filePath.parent_path(), ec);
        if (ec) {
            throw std::runtime_error(std::format("Failed to create directory for file {}", m_filePath.generic_string()));
        }
    }

    if (not openFile()) {
        throw std::runtime_error(std::format("Failed to open file {}", m_filePath.generic_string()));
    }
}

auto FileLogger::log(const std::vector<std::unique_ptr<LogEntry>>& entries) -> void {
    std::error_code ec {};
    const auto size { std::filesystem::file_size(m_filePath, ec) };

    if (ec) {
        throw std::runtime_error(std::format("Failed to get file size of {}: {}", m_filePath.generic_string(), ec.message()));
    }

    if (size > m_maxFileSize) {
        closeFile();
        rotateFile();
        if (not openFile()) {
            throw std::runtime_error(std::format("Failed to reopen file {} after rotation", m_filePath.generic_string()));
        }
    }

    for (auto& entry : entries) {
        m_file << toString(*entry) << '\n';
    }

    if (not entries.empty())
        m_file.flush();
}

auto FileLogger::openFile() -> bool {
    m_file.clear();
    m_file.open(m_filePath, std::ios::app);
    return m_file.is_open();
}

auto FileLogger::closeFile() -> void {
    m_file.close();
}

auto FileLogger::rotateFile() -> void {
    std::string newFileName { m_filePath.stem().generic_string() + std::format("_{}", utils::sanitizeFileName(utils::systemClockToString(std::chrono::system_clock::now(), "_", false))) + m_filePath.extension().generic_string() };
    const auto rotatedPath { m_filePath.parent_path() / newFileName };

    std::error_code ec {};
    std::filesystem::rename(m_filePath, rotatedPath, ec);

    if (ec) {
        throw std::runtime_error(std::format("Failed to rotate file {}: {}", m_filePath.generic_string(), ec.message()));
    }
}

auto makeFileLogger(std::filesystem::path filePath, std::uintmax_t maxFileSize) -> std::expected<std::unique_ptr<Logger>, std::string> {
    try {
        return std::make_unique<FileLogger>(std::move(filePath), maxFileSize);
    } catch (const std::exception& e) {
        return std::unexpected { std::string { e.what() } };
    }
}


}
