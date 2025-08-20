export module audio_library_wrapper;

import std;
import audio_device;
import audio_driver;
import audio_stream_params;
import audio_buffer;

namespace audio_engine::audio_library_wrapper {

export using AudioCallback = std::function<void(audio_buffer::AudioBuffer<float>& inputBuffer, audio_buffer::AudioBuffer<float>& outputBuffer)>;
export using LogCallback = std::function<void(const std::string& log)>;

export class AudioLibraryWrapper {
public:
    explicit AudioLibraryWrapper(const LogCallback& logCallback);
    virtual ~AudioLibraryWrapper() = default;

    [[nodiscard]] virtual auto probeDevices() -> std::expected<std::vector<std::shared_ptr<const audio_device::AudioDevice>>,
        std::string> = 0;
    [[nodiscard]] virtual auto audioDriver() const -> std::expected<audio_driver::AudioDriver, std::string> = 0;
    [[nodiscard]] virtual auto openStream(const audio_stream_params::AudioStreamParams& audioStreamParams,
        const AudioCallback& audioCallback) -> bool = 0;
                  virtual auto closeStream() -> void = 0;
    [[nodiscard]] virtual auto startStream() -> bool = 0;
    [[nodiscard]] virtual auto stopStream() -> bool = 0;
    [[nodiscard]] virtual auto isStreamOpen() const -> bool = 0;
    [[nodiscard]] virtual auto isStreamRunning() const -> bool = 0;

protected:
    LogCallback m_logCallback;
    AudioCallback m_audioCallback;
    std::unique_ptr<audio_buffer::AudioBuffer<float>> m_inputAudioBuffer;
    std::unique_ptr<audio_buffer::AudioBuffer<float>> m_outputAudioBuffer;
};

export template <class T> requires std::derived_from<T, AudioLibraryWrapper>
[[nodiscard]] auto makeAudioLibraryWrapper(const LogCallback& logCallback, audio_driver::AudioDriver audioDriver) -> std::expected<std::unique_ptr<AudioLibraryWrapper>, std::string> {
    return std::make_unique<T>(logCallback, audioDriver);
}

}
