module file_logger;

import utils;

namespace logger {

FileLogger::FileLogger(std::filesystem::path filePath, const std::uintmax_t maxFileSize, std::chrono::milliseconds retentionPeriod)
 :  m_filePath { std::move(filePath) },
    m_maxFileSize { maxFileSize },
    m_file {},
    m_retentionPeriod { retentionPeriod } {

    if (m_maxFileSize == 0) {
        throw std::invalid_argument("Log file size must be greater than 0");
    }

    if (m_retentionPeriod == std::chrono::milliseconds::zero()) {
        throw std::invalid_argument("Log file retention period must be greater than 0");
    }

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

    deleteOldFiles();
}

auto FileLogger::log(std::span<const LogEntry> entries) -> void {
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

    for (const auto& entry : entries) {
        m_file << toString(entry) << '\n';
    }

    m_file.flush();

    if (not m_file.good()) {
        throw std::runtime_error(std::format("Failed to write to log file {}", m_filePath.generic_string()));
    }
}

auto FileLogger::openFile() -> bool {
    m_file.clear();
    m_file.open(m_filePath, std::ios::app);
    return m_file.is_open();
}

auto FileLogger::closeFile() -> void {
    m_file.close();
}

auto FileLogger::rotateFile() const -> void {
    std::string newFileName { m_filePath.stem().generic_string() + std::format("_{}", utils::sanitizeFileName(utils::systemClockToString(std::chrono::system_clock::now(), "_", false))) + m_filePath.extension().generic_string() };
    const auto rotatedPath { m_filePath.parent_path() / newFileName };

    std::error_code ec {};
    std::filesystem::rename(m_filePath, rotatedPath, ec);

    if (ec) {
        throw std::runtime_error(std::format("Failed to rotate file {}: {}", m_filePath.generic_string(), ec.message()));
    }

    // Update last-write time so retention won't delete it immediately
    std::filesystem::last_write_time(
        rotatedPath,
        std::filesystem::file_time_type::clock::now(),
        ec);

    if (ec) {
        throw std::runtime_error(std::format("Failed to update write time for file file {}: {}", m_filePath.generic_string(), ec.message()));
    }

    deleteOldFiles();
}

auto FileLogger::deleteOldFiles() const -> void {
    const auto logDirectory { m_filePath.has_parent_path()? m_filePath.parent_path() : std::filesystem::current_path() };
    const auto now { std::filesystem::file_time_type::clock::now() };

    const auto ext { m_filePath.extension().string() };

    std::vector<std::filesystem::path> candidates {};

    for (std::error_code ec {}; const auto& entry : std::filesystem::directory_iterator { logDirectory, ec }) {
        if (ec)
            break;

        if (std::error_code entryEc {}; not entry.is_regular_file(entryEc) or entryEc)
            continue;

        const auto& path { entry.path() };

        if (const auto filename { path.filename().string() }; not filename.ends_with(ext))
            continue;

        // Skip the current log file
        if (std::error_code equivEc {}; std::filesystem::equivalent(path, m_filePath, equivEc))
            continue;

        candidates.push_back(path);
    }

    for (const auto& path : candidates) {
        std::error_code timeEc {};
        const auto lastWriteTime { std::filesystem::last_write_time(path, timeEc) };
        if (timeEc) continue;

        if (now - lastWriteTime > m_retentionPeriod) {
            std::error_code removeEc {};
            std::filesystem::remove(path, removeEc);
            // Ignore removal errors – retried on the next run
        }
    }
}

auto makeFileLogger(std::filesystem::path filePath, std::uintmax_t maxFileSize, std::chrono::milliseconds retentionPeriod) -> std::expected<std::unique_ptr<Logger>, std::string> {
    try {
        return std::make_unique<FileLogger>(std::move(filePath), maxFileSize, retentionPeriod);
    } catch (const std::exception& e) {
        return std::unexpected { std::string { e.what() } };
    }
}

}