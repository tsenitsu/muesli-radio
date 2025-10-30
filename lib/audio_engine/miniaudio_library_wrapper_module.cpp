module;
#include <miniaudio.h>
export module miniaudio_library_wrapper;

import std;
import audio_device;
import audio_driver;
import audio_library_wrapper;
import audio_stream_params;
import audio_buffer;

namespace audio_engine::audio_library_wrapper {

export class MiniaudioLibraryWrapper final: public AudioLibraryWrapper {
public:
    MiniaudioLibraryWrapper(const LogCallback& logCallback, audio_driver::AudioDriver audioDriver);
    ~MiniaudioLibraryWrapper() override;

    [[nodiscard]] auto probeDevices() -> std::expected<std::vector<std::unique_ptr<const audio_device::AudioDevice>>,
        std::string> override;
    [[nodiscard]] auto audioDriver() const -> std::expected<audio_driver::AudioDriver, std::string> override;
    [[nodiscard]] auto openStream(const audio_stream_params::AudioStreamParams& audioStreamParams, const AudioCallback& audioCallback) -> bool override;
                  auto closeStream() -> void override;
    [[nodiscard]] auto startStream() -> bool override;
    [[nodiscard]] auto stopStream() -> bool override;
    [[nodiscard]] auto isStreamOpen() const -> bool override;
    [[nodiscard]] auto isStreamRunning() const -> bool override;

private:
    friend auto miniaudioLogCallback(void* userData, ma_uint32 logLevel, const char* logMessage) -> void;
    friend auto miniaudioAudioCallback(ma_device* device, void* outputBuffer, const void* inputBuffer, ma_uint32 frameCount) -> void;

    ma_context m_context;
    ma_log  m_log;
    ma_device m_device;
};

template<>
[[nodiscard]] auto makeAudioLibraryWrapper<MiniaudioLibraryWrapper>(const LogCallback& logCallback, audio_driver::AudioDriver audioDriver) -> std::expected<std::unique_ptr<AudioLibraryWrapper>, std::string>;

}