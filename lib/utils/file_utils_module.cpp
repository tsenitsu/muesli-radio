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

}