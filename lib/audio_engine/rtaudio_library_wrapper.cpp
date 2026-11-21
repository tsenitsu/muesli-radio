module;
#include <RtAudio.h>
module rtaudio_library_wrapper;

import logger;

namespace audio_engine::audio_library_wrapper {

namespace {

// RtAudio reports one bitmask of native formats. SINT8 and FLOAT64 have no AudioFormat equivalent
// and are skipped.
auto toNativeDataFormats(const RtAudio::DeviceInfo& info, const audio_device::ChannelCount_t maxChannels) -> std::vector<audio_device::NativeDataFormat> {
    static constexpr std::array formatMap {
        std::pair { RTAUDIO_SINT16,  audio_format::AudioFormat::SignedInt16 },
        std::pair { RTAUDIO_SINT24,  audio_format::AudioFormat::SignedInt24 },
        std::pair { RTAUDIO_SINT32,  audio_format::AudioFormat::SignedInt32 },
        std::pair { RTAUDIO_FLOAT32, audio_format::AudioFormat::Float32 }
    };

    std::vector<audio_device::NativeDataFormat> formats {};

    auto minSampleRate { info.currentSampleRate };
    auto maxSampleRate { info.currentSampleRate };

    if (not info.sampleRates.empty()) {
        const auto [lowest, highest] { std::ranges::minmax(info.sampleRates) };
        minSampleRate = lowest;
        maxSampleRate = highest;
    }

    if (maxSampleRate == 0)
        return formats;

    for (const auto& [rtFormat, format] : formatMap) {
        if ((info.nativeFormats & rtFormat) != 0) {
            formats.emplace_back(audio_device::Flags_t { 0 }, format, audio_device::ChannelCount_t { 1 }, maxChannels, minSampleRate, maxSampleRate);
        }
    }

    return formats;
}

// Device buffers are non-interleaved: channel c starts at c * deviceFrames.
// AudioBuffer::copyFromRawBuffer / writeToRawBuffer are not used here because they assume the raw
// buffer's channel stride equals the number of frames copied, which is false for partial chunks.
auto copyDeviceToBlock(const float* deviceBuffer,
                       const unsigned int deviceFrames,
                       const unsigned int deviceOffset,
                       audio_buffer::AudioBuffer<float>& block,
                       const unsigned int blockPosition,
                       const unsigned int frames) -> void {
    for (auto channel { audio_device::ChannelCount_t { 0 } }; channel < block.numberOfChannels(); ++channel) {
        std::copy_n(deviceBuffer + static_cast<std::size_t>(channel) * deviceFrames + deviceOffset,
                    frames,
                    block.view(channel).m_leftMono.data() + blockPosition);
    }
}

auto copyBlockToDevice(const audio_buffer::AudioBuffer<float>& block,
                       const unsigned int blockPosition,
                       float* deviceBuffer,
                       const unsigned int deviceFrames,
                       const unsigned int deviceOffset,
                       const unsigned int frames) -> void {
    for (auto channel { audio_device::ChannelCount_t { 0 } }; channel < block.numberOfChannels(); ++channel) {
        std::copy_n(block.view(channel).m_leftMono.data() + blockPosition,
                    frames,
                    deviceBuffer + static_cast<std::size_t>(channel) * deviceFrames + deviceOffset);
    }
}

}

RtAudioLibraryWrapper::RtAudioLibraryWrapper(const LogCallback& logCallback, const audio_driver::AudioDriver audioDriver)
 :  AudioLibraryWrapper { logCallback },
    m_rtAudio { nullptr } {

    // Every other driver comes from miniaudio
    if (audioDriver != audio_driver::AudioDriver::Asio) {
        throw std::runtime_error { "The RtAudio wrapper only supports the ASIO driver" };
    }

    m_rtAudio = std::make_unique<RtAudio>(RtAudio::WINDOWS_ASIO, [this](const RtAudioErrorType type, const std::string& errorText) {
        onRtAudioError(type, errorText);
    });

    // If ASIO is not compiled in, RtAudio silently falls back to another API
    if (m_rtAudio->getCurrentApi() != RtAudio::WINDOWS_ASIO) {
        throw std::runtime_error { "ASIO is not available in this RtAudio build" };
    }
}

RtAudioLibraryWrapper::~RtAudioLibraryWrapper() {
    if (m_rtAudio != nullptr and m_rtAudio->isStreamOpen())
        m_rtAudio->closeStream();
}

auto RtAudioLibraryWrapper::probeDevices() -> std::expected<std::vector<std::unique_ptr<const audio_device::AudioDevice>>, std::string> {
    std::vector<std::unique_ptr<const audio_device::AudioDevice>> deviceList {};

    // One broken driver must not hide the others, so problems are logged and the device skipped
    // (unlike the miniaudio wrapper, which fails the whole probe).
    const auto addDevice { [&](const RtAudio::DeviceInfo& info, const audio_device::AudioDeviceType type) -> void {
        const auto isOutput { type == audio_device::AudioDeviceType::Output };
        const auto channels { isOutput ? info.outputChannels : info.inputChannels };

        if (channels == 0)
            return;

        auto deviceResult { audio_device::makeAudioDevice(audio_device::DeviceId { audio_device::RtAudioDeviceId { info.ID } },
                                                          std::string { info.name },
                                                          isOutput ? info.isDefaultOutput : info.isDefaultInput,
                                                          type,
                                                          toNativeDataFormats(info, channels)) };

        if (deviceResult.has_value()) {
            deviceList.emplace_back(std::move(deviceResult).value());
        } else {
            logMessage(logger::LogLevel::Warning, std::format("Skipping {} device {}: {}", isOutput ? "output" : "input", info.name, deviceResult.error()));
        }
    } };

    if (m_rtAudio->isStreamOpen()) {
        // ASIO keeps a single driver loaded and probing loads every driver's DLL:
        // do not touch RtAudio while a stream is open, reuse the last probe instead.
        logMessage(logger::LogLevel::Info, "A stream is open: returning the devices found by the last probe");
    } else {
        m_deviceInfoCache.clear();

        for (const auto deviceId : m_rtAudio->getDeviceIds()) {
            const auto info { m_rtAudio->getDeviceInfo(deviceId) };

            // ID 0 is RtAudio's "invalid device" marker
            if (info.ID == 0) {
                logMessage(logger::LogLevel::Warning, std::format("Can not retrieve device info for RtAudio device {}", deviceId));
                continue;
            }

            m_deviceInfoCache.push_back(info);
        }
    }

    for (const auto& info : m_deviceInfoCache) {
        // A single ASIO driver exposes inputs and outputs: expose it as two devices sharing the same id
        addDevice(info, audio_device::AudioDeviceType::Output);
        addDevice(info, audio_device::AudioDeviceType::Input);
    }

    return deviceList;
}

auto RtAudioLibraryWrapper::audioDriver() const -> std::expected<audio_driver::AudioDriver, std::string> {
    return audio_driver::AudioDriver::Asio;
}

auto RtAudioLibraryWrapper::openStream(const audio_stream_params::AudioStreamParams& audioStreamParams, const AudioCallback& audioCallback) -> bool {
    // Checked here because RtAudio would fail, and the cleanup after a failed open would close the running stream
    if (m_rtAudio->isStreamOpen()) {
        logMessage(logger::LogLevel::Error, "A stream is already open");
        return false;
    }

    // The audio buffers are float: RtAudio converts from the native device format for us.
    if (audioStreamParams.m_format != audio_format::AudioFormat::Float32) {
        logMessage(logger::LogLevel::Error, "RtAudio streams must use the 32-bit float format");
        return false;
    }

    if (audioStreamParams.m_bufferLength == 0) {
        logMessage(logger::LogLevel::Error, "Buffer length must be greater than zero");
        return false;
    }

    const auto* inputParams { dynamic_cast<const audio_stream_params::InputAudioStreamParams*>(&audioStreamParams) };
    const auto* outputParams { dynamic_cast<const audio_stream_params::OutputAudioStreamParams*>(&audioStreamParams) };

    // Loopback parameters are neither: ASIO has no loopback
    if (inputParams == nullptr and outputParams == nullptr) {
        logMessage(logger::LogLevel::Error, "RtAudio streams need input and/or output parameters: loopback is not supported");
        return false;
    }

    // Device ids produced by another audio library can not be used here
    const auto toStreamParameters { [this](const audio_device::DeviceId& deviceId, const audio_device::ChannelCount_t channels, const std::string_view role) -> std::optional<RtAudio::StreamParameters> {
        const auto idResult { deviceId.id<audio_device::RtAudioDeviceId>() };

        if (not idResult.has_value()) {
            logMessage(logger::LogLevel::Error, std::format("{} device: {}", role, idResult.error()));
            return std::nullopt;
        }

        RtAudio::StreamParameters parameters {};
        parameters.deviceId     = idResult->m_value;
        parameters.nChannels    = channels;
        parameters.firstChannel = 0;
        return parameters;
    } };

    std::optional<RtAudio::StreamParameters> inputParameters {};
    std::optional<RtAudio::StreamParameters> outputParameters {};

    if (inputParams != nullptr) {
        inputParameters = toStreamParameters(inputParams->m_inputDeviceId, inputParams->m_numberOfInputChannels, "Input");

        if (not inputParameters.has_value())
            return false;
    }

    if (outputParams != nullptr) {
        outputParameters = toStreamParameters(outputParams->m_outputDeviceId, outputParams->m_numberOfOutputChannels, "Output");

        if (not outputParameters.has_value())
            return false;
    }

    // ASIO always uses two buffers, so the period size (numberOfBuffers) does not apply
    RtAudio::StreamOptions options {};
    options.flags = RTAUDIO_NONINTERLEAVED;

    // The driver may adjust this value to what it supports (e.g. 2048 requested, 512 granted)
    auto bufferFrames { static_cast<unsigned int>(audioStreamParams.m_bufferLength) };

    const auto openResult { m_rtAudio->openStream(outputParameters.has_value() ? &outputParameters.value() : nullptr,
                                                  inputParameters.has_value() ? &inputParameters.value() : nullptr,
                                                  RTAUDIO_FLOAT32,
                                                  audioStreamParams.m_sampleRate,
                                                  &bufferFrames,
                                                  rtAudioCallback,
                                                  this,
                                                  &options) };

    // The details have already been reported through the error callback
    if (openResult != RTAUDIO_NO_ERROR) {
        // When the input side of a duplex stream fails (e.g. it is not the output's ASIO device), RtAudio
        // returns an error but leaves the output side open: close it, or every following openStream() would fail
        if (m_rtAudio->isStreamOpen())
            m_rtAudio->closeStream();

        return false;
    }

    if (bufferFrames == 0) {
        logMessage(logger::LogLevel::Error, "The driver reported a buffer length of zero frames");
        m_rtAudio->closeStream();
        return false;
    }

    // bufferFrames now holds the period the driver really uses. The audio buffers keep the
    // requested length: processDeviceBuffers() stages device frames into blocks of that size.
    m_blockLength   = audioStreamParams.m_bufferLength;
    m_blockPosition = 0;
    m_devicePeriod  = bufferFrames;

    // Period and block length divide each other: a block never ends in the middle of a device chunk,
    // so output can be read right after processing. Otherwise output runs in lockstep with input.
    m_alignedStaging = m_devicePeriod % m_blockLength == 0 or m_blockLength % m_devicePeriod == 0;

    if (m_devicePeriod % m_blockLength != 0) {
        const auto addedLatency { m_alignedStaging ? m_blockLength - m_devicePeriod : m_blockLength };

        logMessage(logger::LogLevel::Info, std::format("Driver period is {} frames and block length is {} frames: audio is buffered, adding {} frames of latency",
                                                       m_devicePeriod, m_blockLength, addedLatency));
    }

    m_inputAudioBuffer = inputParams != nullptr
        ? audio_buffer::makeAudioBuffer<float>(inputParams->m_numberOfInputChannels, m_blockLength)
        : audio_buffer::makeAudioBuffer<float>(0, 0);

    m_outputAudioBuffer = outputParams != nullptr
        ? audio_buffer::makeAudioBuffer<float>(outputParams->m_numberOfOutputChannels, m_blockLength)
        : audio_buffer::makeAudioBuffer<float>(0, 0);

    m_audioCallback = audioCallback;
    return true;
}

auto RtAudioLibraryWrapper::closeStream() -> void {
    if (m_rtAudio->isStreamOpen())
        m_rtAudio->closeStream();
}

auto RtAudioLibraryWrapper::startStream() -> bool {
    // Never touch the block state while the audio thread may be running
    if (not m_rtAudio->isStreamOpen() or m_rtAudio->isStreamRunning())
        return false;

    // Restart from a clean block boundary: a previous stop may have left a partial block behind
    m_blockPosition = 0;
    m_inputAudioBuffer->clear();
    m_outputAudioBuffer->clear();

    return m_rtAudio->startStream() == RTAUDIO_NO_ERROR;
}

auto RtAudioLibraryWrapper::stopStream() -> bool {
    return m_rtAudio->stopStream() == RTAUDIO_NO_ERROR;
}

auto RtAudioLibraryWrapper::isStreamOpen() const -> bool {
    return m_rtAudio->isStreamOpen();
}

auto RtAudioLibraryWrapper::isStreamRunning() const -> bool {
    return m_rtAudio->isStreamRunning();
}

auto RtAudioLibraryWrapper::logMessage(const logger::LogLevel level, const std::string& message) const -> void {
    m_logCallback(logger::makeLogEntry(level, "RtAudio", message));
}

auto RtAudioLibraryWrapper::onRtAudioError(const RtAudioErrorType type, const std::string& errorText) const -> void {
    const auto level { type == RTAUDIO_NO_ERROR ? logger::LogLevel::Info
                     : type == RTAUDIO_WARNING  ? logger::LogLevel::Warning
                                                : logger::LogLevel::Error };
    logMessage(level, errorText);
}

auto RtAudioLibraryWrapper::rtAudioCallback(void* outputBuffer, void* inputBuffer, const unsigned int frameCount,
                                            [[maybe_unused]] const double streamTime,
                                            [[maybe_unused]] const RtAudioStreamStatus status,
                                            void* userData) -> int {
    // inputBuffer / outputBuffer are null for output-only / input-only streams
    static_cast<RtAudioLibraryWrapper*>(userData)->processDeviceBuffers(static_cast<float*>(outputBuffer),
                                                                        static_cast<const float*>(inputBuffer),
                                                                        frameCount);
    return 0;
}

// The driver delivers deviceFrames frames per call, which may differ from the requested block length.
// Input frames are accumulated and m_audioCallback runs only when a full block is available.
// The device always gets exactly deviceFrames frames of output (silence until a block is produced).
auto RtAudioLibraryWrapper::processDeviceBuffers(float* const deviceOutput, const float* const deviceInput, const unsigned int deviceFrames) -> void {
    // The staging mode chosen in openStream() is only valid for that period: with another one, the
    // aligned path could read past the end of the output block. ASIO always delivers the period fixed
    // at open, so this never happens; if it ever does, play silence rather than reading out of bounds.
    if (deviceFrames != m_devicePeriod) {
        if (deviceOutput != nullptr)
            std::fill_n(deviceOutput, static_cast<std::size_t>(deviceFrames) * m_outputAudioBuffer->numberOfChannels(), 0.0f);

        return;
    }

    // Aligned: output is read right after processing, with the lowest possible latency
    // (none at all if the period is a multiple of the block length).
    // Unaligned: output is read in lockstep with input, which is always safe but costs one block.
    for (auto deviceOffset { 0u }; deviceOffset < deviceFrames; ) {
        const auto blockPosition { m_blockPosition };
        const auto frames { std::min(deviceFrames - deviceOffset, m_blockLength - blockPosition) };

        // Unaligned: play what the previous block produced at this same position
        if (deviceOutput != nullptr and not m_alignedStaging)
            copyBlockToDevice(*m_outputAudioBuffer, blockPosition, deviceOutput, deviceFrames, deviceOffset, frames);

        if (deviceInput != nullptr)
            copyDeviceToBlock(deviceInput, deviceFrames, deviceOffset, *m_inputAudioBuffer, blockPosition, frames);

        m_blockPosition += frames;

        if (m_blockPosition == m_blockLength) {
            m_blockPosition = 0;

            m_outputAudioBuffer->clear();
            m_audioCallback(*m_inputAudioBuffer, *m_outputAudioBuffer);
        }

        // Aligned: read from the position the input has just reached
        // (0 right after a block completes; silence until the first block has been produced)
        if (deviceOutput != nullptr and m_alignedStaging)
            copyBlockToDevice(*m_outputAudioBuffer, m_blockPosition, deviceOutput, deviceFrames, deviceOffset, frames);

        deviceOffset += frames;
    }
}

template<>
auto makeAudioLibraryWrapper<RtAudioLibraryWrapper>(const LogCallback& logCallback, audio_driver::AudioDriver audioDriver) -> std::expected<std::unique_ptr<AudioLibraryWrapper>, std::string> {
    std::unique_ptr<RtAudioLibraryWrapper> rtAudioLibraryWrapper { nullptr };

    try {
        rtAudioLibraryWrapper = std::make_unique<RtAudioLibraryWrapper>(logCallback, audioDriver);
    } catch (const std::exception& e) {
        return std::unexpected { std::format("RtAudio library wrapper creation error: {}", e.what()) };
    }

    return rtAudioLibraryWrapper;
}

}