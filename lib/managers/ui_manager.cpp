module ui_manager;

import dropdown;
import audio_driver;
import audio_format;

namespace managers {

using RoutingList = std::vector<std::unique_ptr<audio_engine::audio_mixer::ChannelRouting>>;

auto makeRoutingItems(const RoutingList& routingList) -> std::vector<ui::components::MenuItem> {
    std::vector<ui::components::MenuItem> items;
    items.reserve(routingList.size());
    unsigned int id { 0 };
    std::ranges::transform(routingList, std::back_inserter(items), [&id](const auto& routing) {
        return ui::components::MenuItem { id++, toString(*routing) };
    });
    return items;
}

template <typename Range, typename Proj>
auto makeMenuItems(const Range& range, Proj projection) -> std::vector<ui::components::MenuItem> {
    std::vector<ui::components::MenuItem> items;
    if constexpr (std::ranges::sized_range<Range>) {
        items.reserve(std::ranges::size(range));
    }

    unsigned int id { 0 };
    for (const auto& item : range) {
        items.emplace_back(id++, projection(item));
    }

    return items;
}

UiManager::UiManager(std::expected<AudioEngineManager*, std::string>&& engineManagerResult)
    : m_audioEngineManager { nullptr },
      m_inputAudioDeviceSummaryList {},
      m_outputAudioDeviceSummaryList {},
      m_selectedInputDeviceIndex { std::nullopt },
      m_selectedOutputDeviceIndex { std::nullopt },
      m_selectedBufferLengthIndex { std::nullopt },
      m_selectedSampleFormatIndex { std::nullopt },
      m_inputLevels {},
      m_outputLevels {},
      m_inputRowStates {},
      m_inputRoutingList {},
      m_outputRoutingList {},
      m_mainWindow { std::make_unique<ui::components::MainWindow>() } {

    if (not engineManagerResult.has_value()) {
        m_mainWindow->enableControls(false);
        m_mainWindow->postToast(std::move(engineManagerResult).error());
        return;
    }

    m_audioEngineManager = engineManagerResult.value();
    if (m_audioEngineManager == nullptr) {
        m_mainWindow->enableControls(false);
        m_mainWindow->postToast("An error occurred while trying to initialize the audio engine");
        return;
    }

    const auto currentAudioDriver { m_audioEngineManager->audioDriver().get() };
    if (not currentAudioDriver.has_value()) {
        m_mainWindow->enableControls(false);
        m_mainWindow->postToast(currentAudioDriver.error());
        return;
    }

    const std::pair defaultDriver {
        static_cast<int>(std::distance(
            std::ranges::begin(audio_engine::audio_driver::availableAudioDrivers),
            std::ranges::find(audio_engine::audio_driver::availableAudioDrivers, currentAudioDriver.value()))),
        audio_engine::audio_driver::toString(currentAudioDriver.value()).value()
    };

    setupDriverDropdown(defaultDriver);
    setupDeviceDropdowns();
    setupBufferLengthDropdown();
    setupOpenDevicesButton();
    setupSampleFormatDropdown();
    setupRecordingToggle();
}

auto UiManager::setupDriverDropdown(std::pair<int, std::string> defaultDriver) -> void {
    m_mainWindow->configureAudioDriverDropdown(
        [] {
            return makeMenuItems(
                audio_engine::audio_driver::availableAudioDrivers,
                [](const auto& driver) {
                    return audio_engine::audio_driver::toString(driver).value();
                }
            );
        },
        [this](const unsigned int id, std::string_view) {
            if (auto taskResult { m_audioEngineManager->audioDriver(audio_engine::audio_driver::availableAudioDrivers[id]).get() };
                not taskResult.has_value()) {
                m_mainWindow->postToast(taskResult.error());
                return;
            }
            m_mainWindow->resetAudioDevicesControls();
            m_inputLevels.clear();
            m_outputLevels.clear();
            m_inputAudioDeviceSummaryList.clear();
            m_outputAudioDeviceSummaryList.clear();
            m_selectedInputDeviceIndex = std::nullopt;
            m_selectedOutputDeviceIndex = std::nullopt;
        },
        std::move(defaultDriver));
}

auto UiManager::setupDeviceDropdowns() -> void {
    m_mainWindow->configureInputDeviceDropdown(
        [this] {
            std::vector<ui::components::MenuItem> items;
            if (auto taskResult { m_audioEngineManager->inputAudioDeviceSummaryList().get() };
                not taskResult.has_value()) {
                m_mainWindow->postToast(taskResult.error());
            } else {
                m_inputAudioDeviceSummaryList = std::move(taskResult).value();
                m_inputAudioDeviceSummaryList.emplace_back("No device", audio_engine::audio_device::ChannelCount_t { 0 });
            }

            return makeMenuItems(
                m_inputAudioDeviceSummaryList,
                [](const auto& summary) { return summary.m_deviceName; }
            );
        },
        [this](const unsigned int id, std::string_view) {
            m_selectedInputDeviceIndex = (m_inputAudioDeviceSummaryList[id].m_channels == 0)
                ? std::nullopt
                : std::optional { id };
        });

    m_mainWindow->configureOutputDeviceDropdown(
        [this] {
            std::vector<ui::components::MenuItem> items;
            if (auto taskResult { m_audioEngineManager->outputAudioDeviceSummaryList().get() };
                not taskResult.has_value()) {
                m_mainWindow->postToast(taskResult.error());
            } else {
                m_outputAudioDeviceSummaryList = std::move(taskResult).value();
                m_outputAudioDeviceSummaryList.emplace_back("No device", audio_engine::audio_device::ChannelCount_t { 0 });
            }

            return makeMenuItems(
                m_outputAudioDeviceSummaryList,
                [](const auto& summary) { return summary.m_deviceName; }
            );
        },
        [this](const unsigned int id, std::string_view) {
            m_selectedOutputDeviceIndex = (m_outputAudioDeviceSummaryList[id].m_channels == 0)
                ? std::nullopt
                : std::optional { id };
        });
}

auto UiManager::setupBufferLengthDropdown() -> void {
    m_mainWindow->configureBufferLengthDropdown(
        [this] {
            return makeMenuItems(
                m_audioEngineManager->allowedBufferLengths(),
                [](const auto& bufferLength) { return std::to_string(bufferLength); }
            );
        },
        [this](const unsigned int id, std::string_view) {
            m_selectedBufferLengthIndex = id;
        });
}

auto UiManager::setupOpenDevicesButton() -> void {
    m_mainWindow->configureOpenDevicesButton([this] {
        if (not m_selectedBufferLengthIndex.has_value()) {
            m_mainWindow->postToast("Buffer length not selected");
            return;
        }

        std::vector<float> localInputLevels;
        std::vector<float> localOutputLevels;
        std::vector<InputRowState> localInputRowStates;
        RoutingList localInputRoutingList;
        RoutingList localOutputRoutingList;

        std::optional<std::string> inputDeviceName;
        if (m_selectedInputDeviceIndex.has_value()) {
            const auto& inputSummary { m_inputAudioDeviceSummaryList[m_selectedInputDeviceIndex.value()] };
            inputDeviceName = inputSummary.m_deviceName;
            localInputLevels.assign(inputSummary.m_channels, 0.f);
            localInputRowStates.resize(inputSummary.m_channels);
            localInputRoutingList = audio_engine::audio_mixer::makeRoutingList(
                static_cast<audio_engine::audio_mixer::Routing_t>(inputSummary.m_channels));
        } else {
            localInputRoutingList = audio_engine::audio_mixer::makeRoutingList(0);
        }

        std::optional<std::string> outputDeviceName;
        if (m_selectedOutputDeviceIndex.has_value()) {
            const auto& outputSummary { m_outputAudioDeviceSummaryList[m_selectedOutputDeviceIndex.value()] };
            outputDeviceName = outputSummary.m_deviceName;
            localOutputLevels.assign(outputSummary.m_channels, 0.f);
            localOutputRoutingList = audio_engine::audio_mixer::makeStereoRoutingList(
                static_cast<audio_engine::audio_mixer::Routing_t>(outputSummary.m_channels));
        } else {
            localOutputRoutingList = audio_engine::audio_mixer::makeStereoRoutingList(0);
        }

        auto taskResult { m_audioEngineManager->startStream(
            std::move(inputDeviceName),
            std::move(outputDeviceName),
            m_audioEngineManager->allowedBufferLengths()[m_selectedBufferLengthIndex.value()]).get() };

        if (not taskResult.has_value()) {
            // Failure! The UI and class members remain completely untouched.
            m_mainWindow->postToast(taskResult.error());
            return;
        }

        m_mainWindow->clearInputs();
        m_mainWindow->clearOutputs();
        m_mainWindow->clearMeters();

        m_inputLevels = std::move(localInputLevels);
        m_outputLevels = std::move(localOutputLevels);
        m_inputRowStates = std::move(localInputRowStates);
        m_inputRoutingList = std::move(localInputRoutingList);
        m_outputRoutingList = std::move(localOutputRoutingList);

        m_mainWindow->configureMeters(
            static_cast<unsigned int>(m_inputLevels.size()),
            static_cast<unsigned int>(m_outputLevels.size()),
            [this]() -> std::span<const float> {
                m_audioEngineManager->inputLevels(m_inputLevels);
                return m_inputLevels;
            },
            [this]() -> std::span<const float> {
                m_audioEngineManager->outputLevels(m_outputLevels);
                return m_outputLevels;
            });

        addInputRows();
        addOutputRows();
        m_mainWindow->postToast("Device(s) opened");
    });
}

auto UiManager::addInputRows() -> void {
    for (std::size_t i { 0 }; i < m_inputLevels.size(); ++i) {
        m_mainWindow->addInputRow(
            m_audioEngineManager->inputChannelName(
                static_cast<audio_engine::audio_device::ChannelCount_t>(i)).get(),
            [this, i](std::string channelName) {
                m_audioEngineManager->inputChannelName(
                    std::move(channelName),
                    static_cast<audio_engine::audio_device::ChannelCount_t>(i)).get();
            },
            [this] { return makeRoutingItems(m_inputRoutingList); },
            [this, i](unsigned int id, std::string_view) {
                m_inputRowStates[i].fromRoutingIndex = id;
                const auto& toRouting {
                    m_inputRowStates[i].toRoutingIndex.has_value()
                        ? *m_outputRoutingList[m_inputRowStates[i].toRoutingIndex.value()]
                        : *m_outputRoutingList.back()
                };
                m_audioEngineManager->inputChannelRouting(
                    std::make_pair(*m_inputRoutingList[id], toRouting),
                    static_cast<audio_engine::audio_device::ChannelCount_t>(i));
            },
            [this] { return makeRoutingItems(m_outputRoutingList); },
            [this, i](unsigned int id, std::string_view) {
                m_inputRowStates[i].toRoutingIndex = id;
                if (not m_inputRowStates[i].fromRoutingIndex.has_value()) return; // from not yet selected
                m_audioEngineManager->inputChannelRouting(
                    std::make_pair(
                        *m_inputRoutingList[m_inputRowStates[i].fromRoutingIndex.value()],
                        *m_outputRoutingList[id]),
                    static_cast<audio_engine::audio_device::ChannelCount_t>(i));
            });
    }
}

auto UiManager::addOutputRows() const -> void {
    for (std::size_t i { 0 }; i < m_outputLevels.size() / 2; ++i) {
        m_mainWindow->addOutputRow(
            m_audioEngineManager->outputChannelName(
                static_cast<audio_engine::audio_device::ChannelCount_t>(i)).get(),
            [this, i](std::string channelName) {
                m_audioEngineManager->outputChannelName(
                    std::move(channelName),
                    static_cast<audio_engine::audio_device::ChannelCount_t>(i)).get();
            },
            [this] { return makeRoutingItems(m_outputRoutingList); },
            [this, i](unsigned int id, std::string_view) {
                m_audioEngineManager->outputChannelRouting(
                    *m_outputRoutingList[id],
                    static_cast<audio_engine::audio_device::ChannelCount_t>(i));
            });
    }
}

auto UiManager::setupSampleFormatDropdown() -> void {
    m_mainWindow->configureSampleFormatDropdown(
        [] {
            return makeMenuItems(
                audio_engine::audio_format::availableRecordingFormats,
                [](const auto& format) {
                    return audio_engine::audio_format::toString(format).value();
                }
            );
        },
        [this](const unsigned int id, std::string_view) {
            m_selectedSampleFormatIndex = id;
        });
}

auto UiManager::setupRecordingToggle() const -> void {
    m_mainWindow->configureRecordingToggle([this](const bool toggleOn) {
        if (not m_selectedSampleFormatIndex.has_value()) {
            m_mainWindow->postToast("Sample format not selected");
            return false;
        }

        if (toggleOn) {
            if (auto taskResult { m_audioEngineManager->startRecording(
                    audio_engine::audio_format::availableRecordingFormats[m_selectedSampleFormatIndex.value()]) };
                not taskResult.has_value()) {
                m_mainWindow->postToast(taskResult.error());
                return false;
            }
            m_mainWindow->enableControls(false, true);
            m_mainWindow->postToast("Recording started");
            return true;
        }

        m_audioEngineManager->stopRecording();
        m_mainWindow->enableControls(true);
        m_mainWindow->postToast("Recording stopped");
        return true;
    });
}

auto UiManager::run() const -> void {
    m_mainWindow->run();
}

auto makeUiManager(std::expected<AudioEngineManager*, std::string>&& engineManagerResult) -> std::unique_ptr<UiManager> {
    return std::make_unique<UiManager>(std::move(engineManagerResult));
}

}