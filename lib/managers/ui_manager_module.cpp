export module ui_manager;

import std;

import main_window;
import audio_device;
import audio_engine_manager;
import channel_routing;

namespace managers {

struct InputRowState {
    std::optional<unsigned int> fromRoutingIndex;
    std::optional<unsigned int> toRoutingIndex;
};

export class UiManager final {
public:
    explicit UiManager(std::expected<AudioEngineManager*, std::string>&& engineManagerResult);
    auto run() const -> void;

private:
    auto setupDriverDropdown(std::pair<int, std::string> defaultDriver) -> void;
    auto setupDeviceDropdowns() -> void;
    auto setupBufferLengthDropdown() -> void;
    auto setupOpenDevicesButton() -> void;
    auto setupSampleFormatDropdown() -> void;
    auto setupRecordingToggle() const -> void;
    auto addInputRows() -> void;
    auto addOutputRows() const -> void;

    AudioEngineManager* m_audioEngineManager;
    std::vector<audio_engine::audio_device::AudioDeviceSummary> m_inputAudioDeviceSummaryList;
    std::vector<audio_engine::audio_device::AudioDeviceSummary> m_outputAudioDeviceSummaryList;
    std::optional<unsigned int> m_selectedInputDeviceIndex;
    std::optional<unsigned int> m_selectedOutputDeviceIndex;
    std::optional<unsigned int> m_selectedBufferLengthIndex;
    std::optional<unsigned int> m_selectedSampleFormatIndex;
    std::vector<float> m_inputLevels;
    std::vector<float> m_outputLevels;
    std::vector<InputRowState> m_inputRowStates;
    std::vector<std::unique_ptr<audio_engine::audio_mixer::ChannelRouting>> m_inputRoutingList;
    std::vector<std::unique_ptr<audio_engine::audio_mixer::ChannelRouting>> m_outputRoutingList;
    std::unique_ptr<ui::components::MainWindow> m_mainWindow;
};

export [[nodiscard]] auto makeUiManager(std::expected<AudioEngineManager*, std::string>&& engineManagerResult) -> std::unique_ptr<UiManager>;

}