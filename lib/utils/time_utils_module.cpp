export module utils:time_utils;

import std;

namespace utils {

export [[nodiscard]] auto systemClockToString(std::chrono::system_clock::time_point timeStamp,
    std::string_view separator = " ",
    bool millisecondPrecision = true) -> std::string {

    std::string str {};
    if (millisecondPrecision) {
        const auto ms { std::chrono::floor<std::chrono::milliseconds>(timeStamp) };
        str = std::format(std::locale::classic(), "{:L%d|%b|%Y|%H:%M:%S}", ms);
    } else {
        const auto seconds { std::chrono::floor<std::chrono::seconds>(timeStamp) };
        str = std::format(std::locale::classic(), "{:L%d|%b|%Y|%H:%M:%S}", seconds);
    }

    // Advance by separator size since the replaced content is longer
    for (std::size_t pos {}; (pos = str.find('|', pos)) != std::string::npos; pos += separator.size()) // skip past the just-inserted separator
             str.replace(pos, 1u, separator); // replace 1 char with separator

    return str;
}

} // namespace utils