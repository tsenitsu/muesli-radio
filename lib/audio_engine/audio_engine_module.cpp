export module audio_engine;

export import audio_device;
export import audio_driver;
export import audio_stream_params;
export import audio_library_wrapper;
export import miniaudio_library_wrapper;
export import audio_buffer;
export import audio_mixer;
export import audio_channel;
export import channel_routing;
export import audio_recorder;
export import ring_audio_buffer;
export import audio_format;

import std;

namespace audio_engine {

export class AudioEngine {
public:
    // audioLibraryWrapper must not be null
    AudioEngine(const audio_library_wrapper::LogCallback& logCallback,
                std::unique_ptr<audio_library_wrapper::AudioLibraryWrapper> audioLibraryWrapper);

    virtual ~AudioEngine() = default;

    [[nodiscard]] auto audioDriver(audio_driver::AudioDriver newAudioDriver) -> std::expected<void, std::string>;

    [[nodiscard]] auto audioDriver() const -> std::expected<audio_driver::AudioDriver, std::string> {
        return m_audioLibraryWrapper->audioDriver();
    }

    [[nodiscard]] auto probeDevices() -> std::expected<void, std::string>;

    [[nodiscard]] auto defaultInputAudioDeviceName() const -> std::expected<std::string, std::string>;
    [[nodiscard]] auto defaultOutputAudioDeviceName() const -> std::expected<std::string, std::string>;

    [[nodiscard]] auto inputAudioDeviceSummaryList() -> std::expected<std::vector<audio_device::AudioDeviceSummary>, std::string>;
    [[nodiscard]] auto outputAudioDeviceSummaryList() -> std::expected<std::vector<audio_device::AudioDeviceSummary>, std::string>;

    [[nodiscard]] auto startStream(std::optional<std::string> inputDeviceName,
                                   std::optional<std::string> outputDeviceName,
                                   std::optional<std::string> loopbackDeviceName,
                                   audio_stream_params::BufferLength_t bufferLength,
                                   audio_device::SampleRate_t sampleRate) -> std::expected<void, std::string>;

    [[nodiscard]] auto audioMixer() -> const std::unique_ptr<audio_mixer::AudioMixer<float>>& {
        return m_audioMixer;
    }

    [[nodiscard]] auto startRecording(audio_format::AudioFormat format, std::filesystem::path path) -> std::expected<void, std::string>;

    auto stopRecording() -> void {
        m_isRecording.store(false, std::memory_order_release);
    }

    auto finalizeRecording() -> void;

    [[nodiscard]] auto write() const -> bool;

    [[nodiscard]] static auto allowedSampleRates() -> const auto& { return m_allowedSampleRates; }
    [[nodiscard]] static auto allowedBufferLengths() -> const auto& { return m_allowedBufferLengths; }

    auto inputLevels(std::span<float> inputLevels) const -> void;
    auto outputLevels(std::span<float> outputLevels) const -> void;

protected:
    [[nodiscard]] auto getAudioDevice(const std::string& deviceName, audio_device::AudioDeviceType deviceType) const
                    -> std::expected<std::ranges::borrowed_iterator_t<const std::vector<std::unique_ptr<const audio_device::AudioDevice>> &>, std::string>;

    [[nodiscard]] static auto isSampleRateAllowed(audio_device::SampleRate_t sampleRate) -> bool;
    [[nodiscard]] static auto isBufferLengthAllowed(audio_stream_params::BufferLength_t bufferLength) -> bool;

    [[nodiscard]] auto openStream() const -> bool;
    [[nodiscard]] auto closeStream() const -> bool;

    [[nodiscard]] auto getDefaultAudioDevice(audio_device::AudioDeviceType deviceType) const
                    -> std::expected<std::ranges::borrowed_iterator_t<const std::vector<std::unique_ptr<const audio_device::AudioDevice>> &>, std::string>;

    [[nodiscard]] auto audioDeviceSummaryList(audio_device::AudioDeviceType deviceType) -> std::expected<std::vector<audio_device::AudioDeviceSummary>, std::string>;

    auto processInput(const audio_buffer::AudioBuffer<float>& inputBuffer, const audio_buffer::AudioBuffer<float>& outputBuffer) const -> void;
    auto processOutput(audio_buffer::AudioBuffer<float>& outputBuffer) const -> void;
    auto process(const audio_buffer::AudioBuffer<float>& inputBuffer, audio_buffer::AudioBuffer<float>& outputBuffer) const -> void;

    static constexpr std::array<audio_device::SampleRate_t, 2> m_allowedSampleRates { 44100, 48000 };
    static constexpr auto m_format { audio_format::AudioFormat::Float32 };
    static constexpr auto m_periodSize   { audio_stream_params::PeriodSize_t { 3 } };
    static constexpr std::array<const audio_stream_params::BufferLength_t, 5> m_allowedBufferLengths { 1024, 2048, 4096, 8192, 16384 };

    std::unique_ptr<audio_mixer::AudioMixer<float>> m_audioMixer;
    std::vector<std::unique_ptr<const audio_device::AudioDevice>> m_audioDevices;
    std::unique_ptr<audio_stream_params::AudioStreamParams> m_audioStreamParams;
    std::unique_ptr<audio_buffer::AudioBuffer<float>> m_processedInputBuffer;
    std::unique_ptr<ring_audio_buffer::RingAudioBuffer<float>> m_inputRingAudioBuffer;
    std::unique_ptr<ring_audio_buffer::RingAudioBuffer<float>> m_outputRingAudioBuffer;
    std::atomic_bool m_isRecording;
    std::unique_ptr<audio_recorder::AudioRecorder> m_inputRecorder;
    std::unique_ptr<audio_recorder::AudioRecorder> m_outputRecorder;
    std::vector<std::unique_ptr<audio_buffer::AudioStats<float>>> m_inputAudioStats;
    std::vector<std::unique_ptr<audio_buffer::AudioStats<float>>> m_outputAudioStats;
    std::unique_ptr<audio_library_wrapper::AudioLibraryWrapper> m_audioLibraryWrapper;

private:
    // Returns std::nullopt when deviceName holds no value. label is only used in the error message.
    [[nodiscard]] auto getDeviceSelection(const std::optional<std::string>& deviceName,
                                          audio_device::AudioDeviceType deviceType,
                                          std::string_view label) const
                    -> std::expected<std::optional<audio_stream_params::DeviceSelection>, std::string>;

    // Builds the processing state for the new stream, then stops the running stream, swaps the state in and opens the new one
    [[nodiscard]] auto replaceStream(std::unique_ptr<audio_stream_params::AudioStreamParams> streamParams,
                                     std::optional<audio_device::ChannelCount_t> inputChannelCount,
                                     std::optional<audio_device::ChannelCount_t> outputChannelCount,
                                     audio_stream_params::BufferLength_t bufferLength) -> std::expected<void, std::string>;

    // Returns a null buffer when channelCount is empty
    [[nodiscard]] static auto createRingAudioBuffer(std::optional<audio_device::ChannelCount_t> channelCount,
                                                    const audio_stream_params::AudioStreamParams& streamParams,
                                                    std::string_view label)
                    -> std::expected<std::unique_ptr<ring_audio_buffer::RingAudioBuffer<float>>, std::string>;

    [[nodiscard]] auto createInputRecorder(audio_device::ChannelCount_t channelCount,
                                           audio_format::AudioFormat format,
                                           const std::filesystem::path& path) -> std::expected<void, std::string>;

    // Used for both output and loopback streams; channelCount is the number of device channels
    [[nodiscard]] auto createOutputRecorder(audio_device::ChannelCount_t channelCount,
                                            audio_format::AudioFormat format,
                                            const std::filesystem::path& path,
                                            std::string_view label) -> std::expected<void, std::string>;

    static auto updateAudioStats(const audio_buffer::AudioBuffer<float>& buffer,
                                 const std::vector<std::unique_ptr<audio_buffer::AudioStats<float>>>& audioStats) -> void;

    audio_library_wrapper::LogCallback m_logCallback;
    audio_library_wrapper::AudioCallback m_audioCallback;
};

export [[nodiscard]] auto makeAudioEngine(const audio_library_wrapper::LogCallback& logCallback,
                                          audio_driver::AudioDriver audioDriver = audio_driver::availableAudioDrivers[0])
                    -> std::expected<std::unique_ptr<AudioEngine>, std::string>;

}
