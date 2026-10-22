export module folder_picker;

import std;

namespace ui::components {

export class FolderPicker {
public:
    FolderPicker();
    ~FolderPicker();

    FolderPicker(const FolderPicker&)            = delete;
    FolderPicker& operator=(const FolderPicker&) = delete;
    FolderPicker(FolderPicker&&)                 = delete;
    FolderPicker& operator=(FolderPicker&&)      = delete;

    [[nodiscard]] std::expected<std::optional<std::filesystem::path>, std::string>
    pick(const std::filesystem::path& defaultPath = std::filesystem::path {},
         std::string_view title = "Select folder...", void* parentWindow = nullptr) const;
};

}
