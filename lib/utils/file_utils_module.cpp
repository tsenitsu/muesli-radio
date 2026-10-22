export module utils:file_utils;

import std;

namespace utils {

export [[nodiscard]] auto sanitizeFileName(std::string_view name) -> std::string {
    // Illegal on Windows: \ / : * ? " < > |
    // Illegal on Unix: /
    // Also strip control characters
    static constexpr std::string_view illegal { "\\/:*?\"<>|" };

    std::string result {};
    result.reserve(name.size());

    for (const auto c : name) {
        if (std::iscntrl(static_cast<unsigned char>(c)) or illegal.contains(c)) {
            result += '_';
        } else {
            result += c;
        }
    }

    return result;
}

export auto pathToUtf8(const std::filesystem::path& path) -> std::string {
    return std::ranges::to<std::string>(path.u8string());
}

export auto pathFromUtf8(const std::string_view text) -> std::filesystem::path {
    return std::filesystem::path { std::u8string { text.begin(), text.end() } };
}

export auto truncateWithEllipsis(const std::string_view text, const std::size_t maxChars) -> std::string {
    constexpr std::size_t ellipsisLength { 3 };

    if (text.size() <= maxChars || maxChars <= ellipsisLength)
        return std::string { text };

    return std::string { text.substr(0, maxChars - ellipsisLength) } + "...";
}

}