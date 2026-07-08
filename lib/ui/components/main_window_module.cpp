export module main_window;

import std;

import dropdown;

namespace ui::components {

export class MainWindow final {
public:
    MainWindow();
    ~MainWindow();

    auto run() const -> void;

    auto configureAudioDriverDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection, std::pair<unsigned int, std::string> defaultValue) const -> void;
    auto configureInputDeviceDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection) const -> void;
    auto configureOutputDeviceDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection) const -> void;
    auto configureBufferLengthDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection) const -> void;
    auto configureOpenDevicesButton(std::function<void()> onClick) const -> void;
    auto configureSampleFormatDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection) const -> void;
    auto configureRecordingToggle(std::function<bool(bool)> onToggle) const -> void;
    auto configureMeters(unsigned int inputs, unsigned int outputs, std::function<std::span<const float>()> onTimerCallbackInput, std::function<std::span<const float>()> onTimerCallbackOutput) const -> void;

    auto resetAudioDevicesControls() const -> void;

    auto addInputRow(std::string_view inputName, std::function<void(std::string)> onTextChange,
        std::function<std::vector<MenuItem>()> onInputMenuOpen, std::function<void(unsigned int, std::string_view)> onInputSelection,
        std::function<std::vector<MenuItem>()> onOutputMenuOpen, std::function<void(unsigned int, std::string_view)> onOutputSelection) const -> void;
    auto addOutputRow(std::string_view outputName, std::function<void(std::string)> onTextChange,
            std::function<std::vector<MenuItem>()> onOutputMenuOpen, std::function<void(unsigned int, std::string_view)> onOutputSelection) const -> void;

    auto enableControls(bool enabled, bool allowRecordingButton = false) const -> void;
    auto postToast(std::string_view message) const -> void;
    auto clearInputs() const -> void;
    auto clearOutputs() const -> void;
    auto clearMeters() const -> void;

private:
    // Pimpl is required here due to an MSVC bug where
    // module interface files cause third-party headers to be re-parsed in the context
    // of every consumer of the module, including translation units that never directly
    // depend on those libraries. This re-parsing ignores /external:W0 and
    // /external:anglebrackets, so warnings from visage headers become fatal errors
    // via /WX in unrelated targets such as main.cpp.
    //
    // By hiding all visage types inside an opaque Impl struct defined only in the
    // module implementation unit, the module interface exposes no visage types
    // whatsoever. Consumers therefore never trigger a re-parse of visage headers,
    // sidestepping the MSVC bug entirely.
    //
    // Forward-declaring visage types in the interface (e.g. class ApplicationWindow)
    // is not sufficient — MSVC treats the forward declaration as authoritative and
    // considers the type incomplete even in the implementation unit where the full
    // definition is available via the global module fragment.
    class MainWindowImplementation;
    std::unique_ptr<MainWindowImplementation> m_mainWindowImplementation;
};

}