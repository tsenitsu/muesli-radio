module;
#include <visage/app.h>
#include <visage_widgets/text_editor.h>
module main_window;

import std;

import title_bar;
import colors;
import label;
import dropdown;
import toggle_button;
import button;
import toast_popup;
import level_meter;
import fonts;

namespace ui::components {

class MainWindow::MainWindowImplementation final {
public:
    MainWindowImplementation();

    auto configureAudioDriverDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection, std::pair<unsigned int, std::string> defaultValue) -> void;
    auto configureInputDeviceDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection) -> void;
    auto configureOutputDeviceDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection) -> void;
    auto configureBufferLengthDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection) -> void;
    auto configureOpenDevicesButton(std::function<void()> onClick) -> void;
    auto configureSampleFormatDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection) -> void;
    auto configureRecordingToggle(std::function<bool(bool)> onToggle) -> void;
    auto configureMeters(unsigned int inputs, unsigned int outputs, std::function<std::span<const float>()> onTimerCallbackInput, std::function<std::span<const float>()> onTimerCallbackOutput) -> void;

    auto resetAudioDevicesControls() -> void;

    auto addInputRow(std::string_view inputName, std::function<void(std::string)> onTextChange, std::function<std::vector<MenuItem>()> onInputMenuOpen, std::function<void(unsigned int, std::string_view)> onInputSelection,
        std::function<std::vector<MenuItem>()> onOutputMenuOpen, std::function<void(unsigned int, std::string_view)> onOutputSelection) -> void;
    auto addOutputRow(std::string_view outputName, std::function<void(std::string)> onTextChange,
        std::function<std::vector<MenuItem>()> onOutputMenuOpen, std::function<void(unsigned int, std::string_view)> onOutputSelection) -> void;

    auto enableControls(bool enabled, bool allowRecordingButton = false) -> void;
    auto postToast(std::string_view message) -> void;
    auto clearInputs() -> void;
    auto clearOutputs() -> void;
    auto clearMeters() -> void;

    visage::ApplicationWindow m_applicationWindow;

private:
    auto resize() -> void;
    auto layoutToasts() const -> void;

    TitleBar m_settingsTitleBar;
    Label m_driverLabel;
    DropdownButton m_driverDropdown;
    Label m_inputDeviceLabel;
    DropdownButton m_inputDeviceDropdown;
    Label m_outputDeviceLabel;
    DropdownButton m_outputDeviceDropdown;
    Label m_bufferLengthLabel;
    DropdownButton m_bufferLengthDropdown;
    Button m_openDeviceButton;
    TitleBar m_recordingTitleBar;
    Label m_recordingLabel;
    DropdownButton m_sampleFormatDropdown;
    ToggleButton m_recordingButton;
    TitleBar m_inputTitleBar;
    TitleBar m_outputTitleBar;
    TitleBar m_meteringTitleBar;

    std::vector<std::unique_ptr<visage::TextEditor>> m_inputNames;
    std::vector<std::unique_ptr<Label>> m_inputFromLabels;
    std::vector<std::unique_ptr<DropdownButton>> m_inputFromDropdowns;
    std::vector<std::unique_ptr<Label>> m_inputToLabels;
    std::vector<std::unique_ptr<DropdownButton>> m_inputToDropdowns;

    std::vector<std::unique_ptr<visage::TextEditor>> m_outputNames;
    std::vector<std::unique_ptr<Label>> m_outputToLabels;
    std::vector<std::unique_ptr<DropdownButton>> m_outputToDropdowns;

    std::unique_ptr<LevelMeter> m_inputMeter;
    std::unique_ptr<LevelMeter> m_outputMeter;

    std::vector<std::unique_ptr<ToastPopup>> m_toasts;
    std::vector<std::unique_ptr<ToastPopup>> m_toastToBeDeleted;
};

MainWindow::MainWindow()
 :  m_mainWindowImplementation { std::make_unique<MainWindowImplementation>() } {}

MainWindow::~MainWindow() = default;

auto MainWindow::run() const -> void {
    m_mainWindowImplementation->m_applicationWindow.showMaximized();
    m_mainWindowImplementation->m_applicationWindow.runEventLoop();
}

auto MainWindow::configureAudioDriverDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection, std::pair<unsigned int, std::string> defaultValue) const -> void {
    m_mainWindowImplementation->configureAudioDriverDropdown(std::move(onMenuOpen), std::move(onSelection), std::move(defaultValue));
}

auto MainWindow::configureInputDeviceDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection) const -> void {
    m_mainWindowImplementation->configureInputDeviceDropdown(std::move(onMenuOpen), std::move(onSelection));
}

auto MainWindow::configureOutputDeviceDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection) const -> void {
    m_mainWindowImplementation->configureOutputDeviceDropdown(std::move(onMenuOpen), std::move(onSelection));
}

auto MainWindow::configureBufferLengthDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection) const -> void {
    m_mainWindowImplementation->configureBufferLengthDropdown(std::move(onMenuOpen), std::move(onSelection));
}

auto MainWindow::configureOpenDevicesButton(std::function<void()> onClick) const -> void {
    m_mainWindowImplementation->configureOpenDevicesButton(std::move(onClick));
}

auto MainWindow::configureSampleFormatDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection) const -> void {
    m_mainWindowImplementation->configureSampleFormatDropdown(std::move(onMenuOpen), std::move(onSelection));
}

auto MainWindow::configureRecordingToggle(std::function<bool(bool)> onToggle) const -> void {
    m_mainWindowImplementation->configureRecordingToggle(std::move(onToggle));
}

auto MainWindow::configureMeters(const unsigned int inputs, const unsigned int outputs, std::function<std::span<const float>()> onTimerCallbackInput, std::function<std::span<const float>()> onTimerCallbackOutput) const -> void {
    m_mainWindowImplementation->configureMeters(inputs, outputs, std::move(onTimerCallbackInput), std::move(onTimerCallbackOutput));
}

auto MainWindow::resetAudioDevicesControls() const -> void {
    m_mainWindowImplementation->resetAudioDevicesControls();
}

auto MainWindow::addInputRow(std::string_view inputName, std::function<void(std::string)> onTextChange,
        std::function<std::vector<MenuItem>()> onInputMenuOpen, std::function<void(unsigned int, std::string_view)> onInputSelection,
        std::function<std::vector<MenuItem>()> onOutputMenuOpen, std::function<void(unsigned int, std::string_view)> onOutputSelection) const -> void {

    m_mainWindowImplementation->addInputRow(inputName, std::move(onTextChange), std::move(onInputMenuOpen), std::move(onInputSelection),
    std::move(onOutputMenuOpen), std::move(onOutputSelection));
}

    auto MainWindow::addOutputRow(std::string_view outputName, std::function<void(std::string)> onTextChange,
                std::function<std::vector<MenuItem>()> onOutputMenuOpen, std::function<void(unsigned int, std::string_view)> onOutputSelection) const -> void {
    m_mainWindowImplementation->addOutputRow(outputName, std::move(onTextChange), std::move(onOutputMenuOpen), std::move(onOutputSelection));
}

auto MainWindow::enableControls(const bool enabled, const bool allowRecordingButton) const -> void {
    m_mainWindowImplementation->enableControls(enabled, allowRecordingButton);
}

auto MainWindow::postToast(std::string_view message) const -> void {
    m_mainWindowImplementation->postToast(message);
}

auto MainWindow::clearInputs() const -> void {
    m_mainWindowImplementation->clearInputs();
}

auto MainWindow::clearOutputs() const -> void {
    m_mainWindowImplementation->clearOutputs();
}

auto MainWindow::clearMeters() const -> void {
    m_mainWindowImplementation->clearMeters();
}

MainWindow::MainWindowImplementation::MainWindowImplementation()
 :  m_applicationWindow {},
    m_settingsTitleBar { "Audio settings" },
    m_driverLabel { "Audio driver:" },
    m_driverDropdown { "Select..." },
    m_inputDeviceLabel { "Input device:" },
    m_inputDeviceDropdown { "Select..." },
    m_outputDeviceLabel { "Output device:" },
    m_outputDeviceDropdown { "Select..." },
    m_bufferLengthLabel { "Buffer length:" },
    m_bufferLengthDropdown { "Select..." },
    m_openDeviceButton { "Open device(s)" },
    m_recordingTitleBar { "Recording settings" },
    m_recordingLabel { "Sample format:" },
    m_sampleFormatDropdown { "Select..." },
    m_recordingButton { "Start recording", "Stop recording" },
    m_inputTitleBar { "Inputs" },
    m_outputTitleBar { "Outputs" },
    m_meteringTitleBar { "Metering" },
    m_inputNames { std::vector<std::unique_ptr<visage::TextEditor>> {} },
    m_inputFromLabels { std::vector<std::unique_ptr<Label>> {} },
    m_inputFromDropdowns { std::vector<std::unique_ptr<DropdownButton>> {} },
    m_inputToLabels { std::vector<std::unique_ptr<Label>> {} },
    m_inputToDropdowns { std::vector<std::unique_ptr<DropdownButton>> {} },
    m_outputNames { std::vector<std::unique_ptr<visage::TextEditor>> {} },
    m_outputToLabels { std::vector<std::unique_ptr<Label>> {} },
    m_outputToDropdowns { std::vector<std::unique_ptr<DropdownButton>> {} },
    m_inputMeter { std::make_unique<LevelMeter>(0, nullptr, true) },
    m_outputMeter { std::make_unique<LevelMeter>(0, nullptr, false) },
    m_toasts { std::vector<std::unique_ptr<ToastPopup>> {} } {

    m_applicationWindow.addChild(m_settingsTitleBar);

    m_applicationWindow.addChild(m_driverLabel);
    m_applicationWindow.addChild(m_driverDropdown);

    m_applicationWindow.addChild(m_inputDeviceLabel);
    m_applicationWindow.addChild(m_inputDeviceDropdown);

    m_applicationWindow.addChild(m_outputDeviceLabel);
    m_applicationWindow.addChild(m_outputDeviceDropdown);

    m_applicationWindow.addChild(m_bufferLengthLabel);
    m_applicationWindow.addChild(m_bufferLengthDropdown);

    m_applicationWindow.addChild(m_openDeviceButton);

    m_applicationWindow.addChild(m_recordingTitleBar);

    m_applicationWindow.addChild(m_recordingLabel);
    m_applicationWindow.addChild(m_sampleFormatDropdown);

    m_applicationWindow.addChild(m_recordingButton);

    m_applicationWindow.addChild(m_inputTitleBar);
    m_applicationWindow.addChild(m_outputTitleBar);
    m_applicationWindow.addChild(m_meteringTitleBar);

    m_applicationWindow.addChild(m_inputMeter.get());
    m_applicationWindow.addChild(m_outputMeter.get());

    m_applicationWindow.onResize() = [&] () { resize(); };
    m_applicationWindow.setTitle("Muesli Radio");

    m_applicationWindow.onDraw() = [this] (visage::Canvas& canvas) {
        canvas.setColor(ui::colors::BackgroundColor);
        canvas.fill(0, 0, m_applicationWindow.width(), m_applicationWindow.height());
    };
}

auto MainWindow::MainWindowImplementation::configureAudioDriverDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection, const std::pair<unsigned int, std::string> defaultValue) -> void {
    m_driverDropdown.onMenuOpen(std::move(onMenuOpen));
    m_driverDropdown.onSelection(std::move(onSelection));

    m_driverDropdown.selectedId(defaultValue.first);
    m_driverDropdown.text(defaultValue.second);
}

auto MainWindow::MainWindowImplementation::configureInputDeviceDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection) -> void {
    m_inputDeviceDropdown.onMenuOpen(std::move(onMenuOpen));
    m_inputDeviceDropdown.onSelection(std::move(onSelection));
}

auto MainWindow::MainWindowImplementation::configureOutputDeviceDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection) -> void {
    m_outputDeviceDropdown.onMenuOpen(std::move(onMenuOpen));
    m_outputDeviceDropdown.onSelection(std::move(onSelection));
}

auto MainWindow::MainWindowImplementation::configureBufferLengthDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection) -> void {
    m_bufferLengthDropdown.onMenuOpen(std::move(onMenuOpen));
    m_bufferLengthDropdown.onSelection(std::move(onSelection));
}

auto MainWindow::MainWindowImplementation::configureOpenDevicesButton(std::function<void()> onClick) -> void {
    m_openDeviceButton.onClick(std::move(onClick));
}

auto MainWindow::MainWindowImplementation::configureSampleFormatDropdown(std::function<std::vector<MenuItem>()> onMenuOpen, std::function<void(unsigned int, std::string_view)> onSelection) -> void {
    m_sampleFormatDropdown.onMenuOpen(std::move(onMenuOpen));
    m_sampleFormatDropdown.onSelection(std::move(onSelection));
}

auto MainWindow::MainWindowImplementation::configureRecordingToggle(std::function<bool(bool)> onToggle) -> void {
    m_recordingButton.onToggle(std::move(onToggle));
}

auto MainWindow::MainWindowImplementation::configureMeters(const unsigned int inputs, const unsigned int outputs, std::function<std::span<const float>()> onTimerCallbackInput, std::function<std::span<const float>()> onTimerCallbackOutput) -> void {
    if (m_inputMeter != nullptr)
        m_applicationWindow.removeChild(m_inputMeter.get());

    m_inputMeter = std::make_unique<LevelMeter>(inputs, std::move(onTimerCallbackInput), true);
    m_applicationWindow.addChild(m_inputMeter.get());

    if (m_outputMeter != nullptr)
        m_applicationWindow.removeChild(m_outputMeter.get());

    m_outputMeter = std::make_unique<LevelMeter>(outputs, std::move(onTimerCallbackOutput), false);
    m_applicationWindow.addChild(m_outputMeter.get());

    resize();
}

auto MainWindow::MainWindowImplementation::resetAudioDevicesControls() -> void {
    m_inputDeviceDropdown.reset();
    m_outputDeviceDropdown.reset();
    clearInputs();
    clearOutputs();
    clearMeters();
}

auto MainWindow::MainWindowImplementation::addInputRow(std::string_view inputName, std::function<void(std::string)> onTextChange,
    std::function<std::vector<MenuItem>()> onInputMenuOpen, std::function<void(unsigned int, std::string_view)> onInputSelection,
    std::function<std::vector<MenuItem>()> onOutputMenuOpen, std::function<void(unsigned int, std::string_view)> onOutputSelection) -> void {
    auto name { std::make_unique<visage::TextEditor>() };
    name->setFont(visage::Font(25.0f * 0.5f, fonts::jetbrainsMonoRegular));
    name->setText(inputName.data());
    name->onTextChange().add([name_ptr = name.get(), cb = std::move(onTextChange)] () {
        cb(name_ptr->text().toUtf8());
    });

    m_applicationWindow.addChild(name.get());
    m_inputNames.push_back(std::move(name));

    auto fromLabel { std::make_unique<Label>("In:") };
    m_applicationWindow.addChild(fromLabel.get());
    m_inputFromLabels.push_back(std::move(fromLabel));

    auto fromDrop { std::make_unique<DropdownButton>("Select...") };
    fromDrop->onMenuOpen(std::move(onInputMenuOpen));
    fromDrop->onSelection(std::move(onInputSelection));
    m_applicationWindow.addChild(fromDrop.get());
    m_inputFromDropdowns.push_back(std::move(fromDrop));

    auto toLabel { std::make_unique<Label>("To:") };
    m_applicationWindow.addChild(toLabel.get());
    m_inputToLabels.push_back(std::move(toLabel));

    auto toDrop { std::make_unique<DropdownButton>("Select...") };
    toDrop->onMenuOpen(std::move(onOutputMenuOpen));
    toDrop->onSelection(std::move(onOutputSelection));
    m_applicationWindow.addChild(toDrop.get());
    m_inputToDropdowns.push_back(std::move(toDrop));

    resize();
}

auto MainWindow::MainWindowImplementation::addOutputRow(std::string_view outputName, std::function<void(std::string)> onTextChange,
                std::function<std::vector<MenuItem>()> onOutputMenuOpen, std::function<void(unsigned int, std::string_view)> onOutputSelection) -> void {
    auto name { std::make_unique<visage::TextEditor>() };
    name->setFont(visage::Font(25.0f * 0.5f, fonts::jetbrainsMonoRegular));
    name->setText(outputName.data());
    name->onTextChange().add([name_ptr = name.get(), cb = std::move(onTextChange)] () {
        cb(name_ptr->text().toUtf8());
    });

    m_applicationWindow.addChild(name.get());
    m_outputNames.push_back(std::move(name));

    auto label { std::make_unique<Label>("Out:") };
    m_applicationWindow.addChild(label.get());
    m_outputToLabels.push_back(std::move(label));

    auto drop { std::make_unique<DropdownButton>("Select...") };
    drop->onMenuOpen(std::move(onOutputMenuOpen));
    drop->onSelection(std::move(onOutputSelection));
    m_applicationWindow.addChild(drop.get());
    m_outputToDropdowns.push_back(std::move(drop));

    resize();
}

auto MainWindow::MainWindowImplementation::enableControls(const bool enabled, const bool allowRecordingButton) -> void {
    m_driverDropdown.enabled(enabled);
    m_inputDeviceDropdown.enabled(enabled);
    m_outputDeviceDropdown.enabled(enabled);
    m_bufferLengthDropdown.enabled(enabled);
    m_openDeviceButton.enabled(enabled);

    m_sampleFormatDropdown.enabled(enabled);

    if (allowRecordingButton)
        m_recordingButton.enabled(true);
    else
        m_recordingButton.enabled(enabled);

    auto enableTextEditor { [enabled] (visage::TextEditor* const editor) {
        editor->setActive(enabled);

        // Invert 'enabled' so it ignores mouse events ONLY when disabled
        editor->setIgnoresMouseEvents(!enabled, true);

        if (!enabled) {
            editor->deselect();
            editor->setBackgroundColorId(colors::ComponentBackgroundDisabled);
        } else {
            editor->setBackgroundColorId(visage::TextEditor::TextEditorBackground);
        }

        editor->resized();
    } };

    for (auto& nameEditor : m_inputNames) {
        if (nameEditor)
            enableTextEditor(nameEditor.get());
    }

    for (auto& fromDropdown : m_inputFromDropdowns) {
        if (fromDropdown)
            fromDropdown->enabled(enabled);
    }
    for (auto& toDropdown : m_inputToDropdowns) {
        if (toDropdown)
            toDropdown->enabled(enabled);
    }

    for (auto& nameEditor : m_outputNames) {
        if (nameEditor)
            enableTextEditor(nameEditor.get());
    }
    for (auto& toDropdown : m_outputToDropdowns) {
        if (toDropdown)
            toDropdown->enabled(enabled);
    }

    resize();
}

auto MainWindow::MainWindowImplementation::postToast(std::string_view message) -> void {
    constexpr float toastH { 45.f };
    constexpr float iconSpace { toastH + 5.f };   // [X‑Box] + [Gap]
    constexpr float rightPadding { 20.f };

    const visage::Font messageFont(toastH * 0.45f, fonts::jetbrainsMonoRegular, m_applicationWindow.dpiScale());
    const visage::String visMessage(message.data());
    float textWidth = messageFont.stringWidth(visMessage.c_str(), static_cast<int>(visMessage.length()));

    const float calculatedW = std::clamp(iconSpace + textWidth + rightPadding, 150.f, 1800.f);

    auto toast { std::make_unique<ui::components::ToastPopup>(message, 5000) };
    toast->setBounds(0.0f, 0.0f, calculatedW, toastH);

    toast->onDismiss([this](ToastPopup* t) {
        const auto it { std::ranges::find_if(m_toasts, [t](const auto& p)  {
            return p.get() == t;
        }) };

        if (it != m_toasts.end()) {
            m_applicationWindow.removeChild(t);

            m_toastToBeDeleted.clear();
            m_toastToBeDeleted.push_back(std::move(*it));

            m_toasts.erase(it);
            layoutToasts();
        }
    });

    m_applicationWindow.addChild(toast.get());
    m_toasts.push_back(std::move(toast));

    layoutToasts();
}

auto MainWindow::MainWindowImplementation::resize() -> void {
    const float totalW { m_applicationWindow.width() };

    // Protect against asynchronous mapping traps on Linux
    if (totalW <= 0.0f || m_applicationWindow.height() <= 0.0f) {
        return;
    }

    constexpr float titleH { 30.0f };
    constexpr float padding { 10.0f };
    constexpr float labelH { 25.0f };
    constexpr float dropdownH { 35.0f };
    constexpr float verticalGap { 5.0f };

    const float slotW { totalW / 5.0f };

    m_settingsTitleBar.setBounds(0, 0, totalW, titleH);

    constexpr float settingsY { titleH + padding };

    // Set column using explicit x and w
    auto setColumn = [&](const float x, const float w, visage::Frame* label, visage::Frame* control) {
        if (label) {
            label->setBounds(x + padding, settingsY, w - (padding * 2.0f), labelH);
        }
        if (control) {
            control->setBounds(x + padding, settingsY + labelH + verticalGap, w - (padding * 2.0f), dropdownH);
        }
    };

    const float smallSlotW { slotW * 0.75f };
    const float largeSlotW { slotW * 1.25f };

    setColumn(0.0f, smallSlotW, &m_driverLabel, &m_driverDropdown);
    setColumn(smallSlotW, largeSlotW, &m_inputDeviceLabel, &m_inputDeviceDropdown);
    setColumn(slotW * 2.0f, largeSlotW, &m_outputDeviceLabel, &m_outputDeviceDropdown);
    setColumn(slotW * 2.0f + largeSlotW, smallSlotW, &m_bufferLengthLabel, &m_bufferLengthDropdown);
    setColumn(slotW * 4.0f, slotW, nullptr, &m_openDeviceButton);

    constexpr float recordingSectionY { settingsY + labelH + verticalGap + dropdownH + (padding * 2) };
    m_recordingTitleBar.setBounds(0, recordingSectionY, totalW, titleH);

    constexpr float recControlsY { recordingSectionY + titleH + padding };

    m_recordingLabel.setBounds(padding, recControlsY, slotW - (padding * 2), labelH);
    m_sampleFormatDropdown.setBounds(padding, recControlsY + labelH + verticalGap, slotW - (padding * 2), dropdownH);
    m_recordingButton.setBounds(slotW + padding, recControlsY + labelH + verticalGap, slotW - (padding * 2), dropdownH);

    constexpr float ioSectionY { recControlsY + labelH + verticalGap + dropdownH + (padding * 2) };
    const float thirdColW { totalW / 3.0f };

    m_inputTitleBar.setBounds(0, ioSectionY, thirdColW, titleH);
    m_outputTitleBar.setBounds(thirdColW, ioSectionY, thirdColW, titleH);
    m_meteringTitleBar.setBounds(thirdColW * 2.0f, ioSectionY, thirdColW, titleH);

    constexpr float ioContentY { ioSectionY + titleH + padding };

    const float meterColumnX { thirdColW * 2.0f };
    const float halfMeterAreaW { (thirdColW - (padding * 2.0f)) / 2.0f };

    auto getMeterHeight = [](const size_t channels) {
        const size_t safeChannels { std::max<size_t>(2, channels) };
        constexpr float barH { 12.0f };
        constexpr float pad { 1.0f };
        return (static_cast<float>(safeChannels) * barH) + ((static_cast<float>(safeChannels) + 1.0f) * pad);
    };

    if (m_inputMeter != nullptr) {
        const float inMeterH { getMeterHeight(m_inputMeter->channelCount()) };
        m_inputMeter->setBounds(meterColumnX + padding, ioContentY, halfMeterAreaW, inMeterH);
    }

    if (m_outputMeter != nullptr) {
        const float outMeterH { getMeterHeight(m_outputMeter->channelCount()) };
        m_outputMeter->setBounds(meterColumnX + padding + halfMeterAreaW, ioContentY, halfMeterAreaW, outMeterH);
    }

    constexpr float mixerStartY { ioContentY };
    const float rowW { thirdColW - (padding * 2.0f) };
    constexpr float rowH { 30.0f };
    constexpr float sectionSpacing { 8.0f };
    constexpr float innerGap { 2.0f };
    constexpr float labelW { 35.0f };

    const float inputSharedComponentW { (rowW - (labelW * 2.0f) - (sectionSpacing * 2.0f) - (innerGap * 2.0f)) / 3.0f };

    for (size_t i { 0 }; i < m_inputNames.size(); ++i) {
        const float y { mixerStartY + (static_cast<float>(i) * (rowH + verticalGap)) };
        float x { padding };

        m_inputNames[i]->setBounds(x, y, inputSharedComponentW + 40.0f, rowH);
        x += inputSharedComponentW + 40.0f + sectionSpacing;
        m_inputFromLabels[i]->setBounds(x, y, labelW, rowH);
        x += labelW + innerGap;
        m_inputFromDropdowns[i]->setBounds(x, y, inputSharedComponentW - 20.0f, rowH);
        x += inputSharedComponentW - 20.0f + sectionSpacing;
        m_inputToLabels[i]->setBounds(x, y, labelW, rowH);
        x += labelW + innerGap;
        m_inputToDropdowns[i]->setBounds(x, y, inputSharedComponentW - 20.0f, rowH);
    }

    for (size_t i { 0 }; i < m_outputNames.size(); ++i) {
        const float y { mixerStartY + (static_cast<float>(i) * (rowH + verticalGap)) };
        float x { thirdColW + padding };

        m_outputNames[i]->setBounds(x, y, inputSharedComponentW + 40.0f, rowH);
        x += inputSharedComponentW + 40.0f + sectionSpacing;
        m_outputToLabels[i]->setBounds(x, y, labelW, rowH);
        x += labelW + innerGap;
        m_outputToDropdowns[i]->setBounds(x, y, inputSharedComponentW - 20.0f, rowH);
    }

    layoutToasts();
}

auto MainWindow::MainWindowImplementation::layoutToasts() const -> void {
    constexpr float margin { 20.f };
    constexpr float toastH { 45.f };

    float currentY { m_applicationWindow.height() - margin - toastH };

    for (auto it { m_toasts.rbegin() }; it != m_toasts.rend(); ++it) {
        constexpr float gap { 10.f };
        const float dynamicW { (*it)->width() };
        const float x { m_applicationWindow.width() - dynamicW - margin };
        (*it)->setBounds(x, currentY, dynamicW, toastH);
        currentY -= (toastH + gap);
    }
}

auto MainWindow::MainWindowImplementation::clearInputs() -> void {
    for (auto& nameEditor : m_inputNames) {
        m_applicationWindow.removeChild(nameEditor.get());
    }
    m_inputNames.clear();

    for (auto& fromLabel : m_inputFromLabels) {
        m_applicationWindow.removeChild(fromLabel.get());
    }
    m_inputFromLabels.clear();

    for (auto& fromDropdown : m_inputFromDropdowns) {
        m_applicationWindow.removeChild(fromDropdown.get());
    }
    m_inputFromDropdowns.clear();

    for (auto& toLabel : m_inputToLabels) {
        m_applicationWindow.removeChild(toLabel.get());
    }
    m_inputToLabels.clear();

    for (auto& toDropdown : m_inputToDropdowns) {
        m_applicationWindow.removeChild(toDropdown.get());
    }
    m_inputToDropdowns.clear();

    resize();
}

auto MainWindow::MainWindowImplementation::clearOutputs() -> void {
    for (auto& nameEditor : m_outputNames) {
        m_applicationWindow.removeChild(nameEditor.get());
    }
    m_outputNames.clear();

    for (auto& toLabel : m_outputToLabels) {
        m_applicationWindow.removeChild(toLabel.get());
    }
    m_outputToLabels.clear();

    for (auto& toDropdown : m_outputToDropdowns) {
        m_applicationWindow.removeChild(toDropdown.get());
    }
    m_outputToDropdowns.clear();

    resize();
}

auto MainWindow::MainWindowImplementation::clearMeters() -> void {
    if (m_inputMeter)
        m_applicationWindow.removeChild(*m_inputMeter);

    m_inputMeter.reset();

    if (m_outputMeter)
        m_applicationWindow.removeChild(*m_outputMeter);

    m_outputMeter.reset();

    resize();
}

}
