module;
#include <miniaudio.h>
module miniaudio_library_wrapper;

namespace audio_engine::audio_library_wrapper {

auto miniaudioLogCallback(void* userData, [[maybe_unused]] ma_uint32 logLevel, const char* logMessage) -> void {
    static_cast<MiniaudioLibraryWrapper*>(userData)->m_logCallback(std::string { "miniaudio: " }.append(std::string { logMessage }));
}

MiniaudioLibraryWrapper::MiniaudioLibraryWrapper(const LogCallback& logCallback, const audio_driver::AudioDriver audioDriver)
 :  AudioLibraryWrapper { logCallback },
    m_context {},
    m_log {},
    m_device {} {

    const auto backendResult { audio_driver::toBackend(audioDriver) };
    const std::array backendList { backendResult.value() };

    if (ma_log_init(nullptr, &m_log) != MA_SUCCESS) {
        throw std::runtime_error { "Error initializing miniaudio log" };
    }

    if (ma_log_register_callback(&m_log, ma_log_callback_init(miniaudioLogCallback, this)) != MA_SUCCESS) {
        throw std::runtime_error { "Error registering miniaudio log callback" };
    }

    ma_context_config contextConfig { ma_context_config_init() };
    contextConfig.pLog = &m_log;

    if (ma_context_init(backendList.data(), static_cast<ma_uint32>(backendList.size()), &contextConfig, &m_context) != MA_SUCCESS) {
        throw std::runtime_error { "Error initializing miniaudio context" };
    }
}

MiniaudioLibraryWrapper::~MiniaudioLibraryWrapper() {
    ma_device_uninit(&m_device);
    ma_log_uninit(&m_log);

    if (ma_context_uninit(&m_context) != MA_SUCCESS)
        m_logCallback("Failed to uninitialize miniaudio context\n");
}

auto MiniaudioLibraryWrapper::probeDevices() -> std::expected<std::vector<std::unique_ptr<const audio_device::AudioDevice> >, std::string> {
    ma_device_info* outputDevices;
    ma_uint32 outputDeviceCount;

    ma_device_info* inputDevices;
    ma_uint32 inputDeviceCount;

    if (ma_context_get_devices(&m_context, &outputDevices, &outputDeviceCount, &inputDevices, &inputDeviceCount) != MA_SUCCESS) {
        return std::unexpected { std::string { "Can not retrieve devices from miniaudio context" } };
    }

    std::vector<std::unique_ptr<const audio_device::AudioDevice>> deviceList {};

    for (ma_uint32 i { 0 }; i < outputDeviceCount; ++i) {
        if (ma_context_get_device_info(&m_context, ma_device_type_playback, &outputDevices[i].id, &outputDevices[i]) != MA_SUCCESS) {
            return std::unexpected { std::format("Can not retrieve device info for output device {}", outputDevices[i].name) };
        }

        std::vector<audio_device::NativeDataFormat> formats {};

        for (const auto [format, channels, sampleRate, flags]: std::span { outputDevices[i].nativeDataFormats, outputDevices[i].nativeDataFormatCount }) {
            const auto formatResult { audio_format::toAudioFormat(format) };

            if (not formatResult.has_value()) {
                return std::unexpected { std::string { std::format("Unknown format for output device {}", outputDevices[i].name) } };
            }

            formats.emplace_back(audio_device::NativeDataFormat { formatResult.value(), channels, sampleRate, flags });
        }

        if (formats.empty()) {
            return std::unexpected { std::string { std::format("No formats for output device {}", outputDevices[i].name) } };
        }

        if (auto outputDeviceResult { audio_device::makeAudioDevice(audio_device::DeviceId { outputDevices[i].id }, std::string { outputDevices[i].name }, outputDevices[i].isDefault, audio_device::AudioDeviceType::Output, std::move(formats)) }; outputDeviceResult.has_value()) {
            deviceList.emplace_back(std::move(outputDeviceResult.value()));
        } else {
            return std::unexpected { outputDeviceResult.error() };
        }
    }

    for (ma_uint32 i { 0 }; i < inputDeviceCount; ++i) {
        if (ma_context_get_device_info(&m_context, ma_device_type_capture, &inputDevices[i].id, &inputDevices[i]) != MA_SUCCESS) {
            return std::unexpected { std::format("Can not retrieve device info for input device {}", inputDevices[i].name) };
        }

        std::vector<audio_device::NativeDataFormat> formats {};

        for (const auto [format, channels, sampleRate, flags]: std::span { inputDevices[i].nativeDataFormats, inputDevices[i].nativeDataFormatCount }) {
            const auto formatResult { audio_format::toAudioFormat(format) };

            if (not formatResult.has_value()) {
                return std::unexpected { std::string { std::format("Unknown format for input device {}", inputDevices[i].name) } };
            }

            formats.emplace_back(audio_device::NativeDataFormat { formatResult.value(), channels, sampleRate, flags });
        }

        if (formats.empty()) {
            return std::unexpected { std::string { std::format("No formats for input device {}", inputDevices[i].name) } };
        }

        if (auto inputDeviceResult { audio_device::makeAudioDevice(audio_device::DeviceId { inputDevices[i].id }, std::string { inputDevices[i].name }, inputDevices[i].isDefault, audio_device::AudioDeviceType::Input, std::move(formats)) }; inputDeviceResult.has_value()) {
            deviceList.emplace_back(std::move(inputDeviceResult.value()));
        } else {
            return std::unexpected { inputDeviceResult.error() };
        }
    }

    return deviceList;
}

auto MiniaudioLibraryWrapper::audioDriver() const -> std::expected<audio_driver::AudioDriver, std::string> {
    return audio_driver::toAudioDriver(m_context.backend);
}

auto miniaudioAudioCallback(ma_device* device, void* outputBuffer, const void* inputBuffer, ma_uint32 frameCount) -> void {
    const auto miniaudio { static_cast<MiniaudioLibraryWrapper*>(device->pUserData) };

    miniaudio->m_inputAudioBuffer->copyFromRawBuffer(static_cast<const float*>(inputBuffer), device->capture.channels, frameCount);
    miniaudio->m_outputAudioBuffer->clear();
    miniaudio->m_audioCallback(*miniaudio->m_inputAudioBuffer, *miniaudio->m_outputAudioBuffer);
    miniaudio->m_outputAudioBuffer->writeToRawBuffer(static_cast<float*>(outputBuffer), device->playback.channels, frameCount);
}

auto MiniaudioLibraryWrapper::openStream(const audio_stream_params::AudioStreamParams& audioStreamParams, const AudioCallback& audioCallback) -> bool {
    ma_device_id inputDeviceId {};
    ma_device_id outputDeviceId {};

    ma_device_config deviceConfig {};
    // At this point we do not know the device type: it does not matter because
    // we can override it later on
    deviceConfig = ma_device_config_init(ma_device_type_duplex);

    deviceConfig.dataCallback       = miniaudioAudioCallback;
    deviceConfig.sampleRate         = audioStreamParams.m_sampleRate;
    deviceConfig.periodSizeInFrames = audioStreamParams.m_bufferLength;
    deviceConfig.periods            = audioStreamParams.m_periodSize;
    deviceConfig.pUserData          = this;

    m_inputAudioBuffer = audio_buffer::makeAudioBuffer<float>(0, 0);
    m_outputAudioBuffer = audio_buffer::makeAudioBuffer<float>(0, 0);

    try {
        const auto& inputParams { dynamic_cast<const audio_stream_params::InputAudioStreamParams&>(audioStreamParams) };

        m_inputAudioBuffer = audio_buffer::makeAudioBuffer<float>(inputParams.m_numberOfInputChannels, inputParams.m_bufferLength);

        inputDeviceId = inputParams.m_inputDeviceId.id();
        deviceConfig.capture.pDeviceID  = &inputDeviceId;

        deviceConfig.capture.format     = audio_format::toMaFormat(inputParams.m_format).value();
        deviceConfig.capture.channels   = inputParams.m_numberOfInputChannels;
        deviceConfig.capture.shareMode  = ma_share_mode_shared;
        deviceConfig.deviceType         = ma_device_type_capture;
    } catch ([[maybe_unused]] const std::bad_cast& e) {}

    try {
        const auto& outputParams { dynamic_cast<const audio_stream_params::OutputAudioStreamParams&>(audioStreamParams) };

        m_outputAudioBuffer = audio_buffer::makeAudioBuffer<float>(outputParams.m_numberOfOutputChannels, outputParams.m_bufferLength);

        outputDeviceId = outputParams.m_outputDeviceId.id();
        deviceConfig.playback.pDeviceID = &outputDeviceId;

        deviceConfig.playback.format    = audio_format::toMaFormat(outputParams.m_format).value();
        deviceConfig.playback.channels  = outputParams.m_numberOfOutputChannels;
        deviceConfig.playback.shareMode = ma_share_mode_shared;
        deviceConfig.deviceType         = deviceConfig.deviceType == ma_device_type_capture ? ma_device_type_duplex : ma_device_type_playback;
    } catch ([[maybe_unused]] const std::bad_cast& e) {}

    if (const auto deviceInitResult { ma_device_init(&m_context, &deviceConfig, &m_device) }; deviceInitResult != MA_SUCCESS) {
        return false;
    };

    m_audioCallback = audioCallback;
    return true;
}

auto MiniaudioLibraryWrapper::closeStream() -> void {
    ma_device_uninit(&m_device);
}

auto MiniaudioLibraryWrapper::startStream() -> bool {
    return ma_device_start(&m_device) == MA_SUCCESS;
}

auto MiniaudioLibraryWrapper::stopStream() -> bool {
    return ma_device_stop(&m_device) == MA_SUCCESS;
}

auto MiniaudioLibraryWrapper::isStreamOpen() const -> bool {
    return ma_device_get_state(&m_device) != ma_device_state_uninitialized;
}

auto MiniaudioLibraryWrapper::isStreamRunning() const -> bool {
    const auto deviceState { ma_device_get_state(&m_device) };
    return deviceState == ma_device_state_started or deviceState == ma_device_state_starting;
}

template<>
auto makeAudioLibraryWrapper<MiniaudioLibraryWrapper>(const LogCallback& logCallback, audio_driver::AudioDriver audioDriver) -> std::expected<std::unique_ptr<AudioLibraryWrapper>, std::string> {
    auto errorMessage { std::string { "Miniaudio library wrapper creation error:" } };

    if (const auto api { toBackend(audioDriver) }; not api.has_value()) {
        return std::unexpected { std::format("{} {}", errorMessage, api.error())};
    }

    std::unique_ptr<MiniaudioLibraryWrapper> miniaudioLibraryWrapper { nullptr };

    try {
        miniaudioLibraryWrapper = std::make_unique<MiniaudioLibraryWrapper>(logCallback, audioDriver);
    } catch (const std::exception& e) {
        return std::unexpected { std::format("{} {}", errorMessage, e.what()) };
    }

    return miniaudioLibraryWrapper;
}

}