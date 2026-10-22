module;
#include <nfd.hpp>
module folder_picker;

import utils;

namespace ui::components {

static std::string lastError() {
    const char* err { NFD::GetError() };
    return err? err : std::string { "unknown NFD error" };
}

static nfdwindowhandle_t toNfdHandle(void* native) noexcept {
    nfdwindowhandle_t handle {};

    if (!native) {
        return handle;
    }

#if defined(_WIN32)
    handle.type = NFD_WINDOW_HANDLE_TYPE_WINDOWS;
#elif defined(__APPLE__)
    handle.type = NFD_WINDOW_HANDLE_TYPE_COCOA;
#else
    handle.type = NFD_WINDOW_HANDLE_TYPE_X11;
#endif
    handle.handle = native;

    return handle;
}

FolderPicker::FolderPicker() {
    if (NFD::Init() != NFD_OKAY) {
        throw std::runtime_error { lastError() };
    }
}

FolderPicker::~FolderPicker() {
    NFD::Quit();
}

std::expected<std::optional<std::filesystem::path>, std::string>
FolderPicker::pick(const std::filesystem::path& defaultPath, std::string_view title, void* parentWindow) const {
    // NFD needs null-terminated UTF-8. path::string() is the ANSI code page on
    // Windows, and string_view::data() isn't guaranteed to be null-terminated.
    const std::string defaultPathStr { utils::pathToUtf8(defaultPath) };
    const std::string titleStr { title };

    // Raw overload: the UniquePath overloads don't accept a title.
    switch (nfdu8char_t* raw { nullptr };
            NFD::PickFolder(raw,
                            defaultPathStr.empty() ? nullptr : defaultPathStr.c_str(),
                            toNfdHandle(parentWindow),
                            titleStr.c_str())) {
        case NFD_OKAY: {
            const NFD::UniquePathU8 path { raw };  // freed at scope exit
            return std::filesystem::path { std::ranges::to<std::u8string>(std::string_view { path.get() }) };
        }
        case NFD_CANCEL:
            return std::nullopt;
        default:
            return std::unexpected { lastError() };
    }
}

}