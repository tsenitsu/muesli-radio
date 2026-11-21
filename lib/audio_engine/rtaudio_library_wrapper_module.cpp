module;
#include <RtAudio.h>
export module rtaudio_library_wrapper;

import std;
import audio_buffer;
import audio_device;
import audio_driver;
import audio_format;
import audio_stream_params;
import audio_library_wrapper;
import logger;

namespace audio_engine::audio_library_wrapper {

export class RtAudioLibraryWrapper final : public AudioLibraryWrapper {
public:
    RtAudioLibraryWrapper(const LogCallback& logCallback, audio_driver::AudioDriver audioDriver);
    ~RtAudioLibraryWrapper() override;

    RtAudioLibraryWrapper(const RtAudioLibraryWrapper&) = delete;
    auto operator=(const RtAudioLibraryWrapper&) -> RtAudioLibraryWrapper& = delete;

    [[nodiscard]] auto probeDevices() -> std::expected<std::vector<std::unique_ptr<const audio_device::AudioDevice>>, std::string> override;
    [[nodiscard]] auto audioDriver() const -> std::expected<audio_driver::AudioDriver, std::string> override;
    [[nodiscard]] auto openStream(const audio_stream_params::AudioStreamParams& audioStreamParams, const AudioCallback& audioCallback) -> bool override;
                  auto closeStream() -> void override;
    [[nodiscard]] auto startStream() -> bool override;
    [[nodiscard]] auto stopStream() -> bool override;
    [[nodiscard]] auto isStreamOpen() const -> bool override;
    [[nodiscard]] auto isStreamRunning() const -> bool override;

private:
    static auto rtAudioCallback(void* outputBuffer, void* inputBuffer, unsigned int frameCount,
                                double streamTime, RtAudioStreamStatus status, void* userData) -> int;

    auto processDeviceBuffers(float* deviceOutput, const float* deviceInput, unsigned int deviceFrames) -> void;
    auto logMessage(logger::LogLevel level, const std::string& message) const -> void;
    auto onRtAudioError(RtAudioErrorType type, const std::string& errorText) const -> void;

    std::unique_ptr<RtAudio> m_rtAudio;

    // Result of the last probe. ASIO must not be probed while a stream is open.
    std::vector<RtAudio::DeviceInfo> m_deviceInfoCache;

    // Block staging. The audio thread owns these while the stream runs; they are only
    // written from openStream()/startStream() while it is stopped.
    audio_stream_params::BufferLength_t m_blockLength { 0 };
    audio_stream_params::BufferLength_t m_blockPosition { 0 };

    // Driver period and staging mode, both fixed by openStream()
    audio_stream_params::BufferLength_t m_devicePeriod { 0 };
    bool m_alignedStaging { false };
};

template<>
[[nodiscard]] auto makeAudioLibraryWrapper<RtAudioLibraryWrapper>(const LogCallback& logCallback, audio_driver::AudioDriver audioDriver) -> std::expected<std::unique_ptr<AudioLibraryWrapper>, std::string>;

}