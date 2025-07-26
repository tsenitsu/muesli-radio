export module audio_stream_params;

import std;
import audio_device;
import audio_format;

namespace audio_engine::audio_stream_params {

export using BufferLength_t = unsigned int;
export using PeriodSize_t = unsigned int;

export class AudioStreamParams {
public:
    AudioStreamParams(audio_device::SampleRate_t sampleRate,
              audio_format::AudioFormat format,
              BufferLength_t bufferLength,
              PeriodSize_t periodSize);

    virtual ~AudioStreamParams() = default;

    audio_device::SampleRate_t m_sampleRate;
    audio_format::AudioFormat m_format;
    BufferLength_t m_bufferLength;
    PeriodSize_t m_periodSize;

protected:
    AudioStreamParams() = default;
};

export class InputAudioStreamParams: virtual public AudioStreamParams {
public:
    InputAudioStreamParams(audio_device::SampleRate_t sampleRate,
            audio_format::AudioFormat format,
            BufferLength_t bufferLength,
            PeriodSize_t periodSize,
            const audio_device::DeviceId& inputDeviceId,
            audio_device::ChannelCount_t numberOfInputChannels);

    audio_device::DeviceId m_inputDeviceId;
    audio_device::ChannelCount_t m_numberOfInputChannels;

protected:
    InputAudioStreamParams(const audio_device::DeviceId& inputDeviceId,
            audio_device::ChannelCount_t numberOfInputChannels);
};

export class OutputAudioStreamParams: virtual public AudioStreamParams {
public:
    OutputAudioStreamParams(audio_device::SampleRate_t sampleRate,
                audio_format::AudioFormat format,
                BufferLength_t bufferLength,
                PeriodSize_t periodSize,
                const audio_device::DeviceId& outputDeviceId,
                audio_device::ChannelCount_t numberOfOutputChannels);

    audio_device::DeviceId m_outputDeviceId;
    audio_device::ChannelCount_t m_numberOfOutputChannels;

protected:
    OutputAudioStreamParams(const audio_device::DeviceId& outputDeviceId,
                audio_device::ChannelCount_t numberOfOutputChannels);
};

export class DuplexAudioStreamParams final: public InputAudioStreamParams,
                                      public OutputAudioStreamParams {
public:
    DuplexAudioStreamParams(audio_device::SampleRate_t sampleRate,
            audio_format::AudioFormat format,
            BufferLength_t bufferLength,
            PeriodSize_t periodSize,
            const audio_device::DeviceId& inputDeviceId,
            audio_device::ChannelCount_t numberOfInputChannels,
            const audio_device::DeviceId& outputDeviceId,
            audio_device::ChannelCount_t numberOfOutputChannels);
};

export [[nodiscard]] auto makeAudioStreamParams(audio_device::SampleRate_t sampleRate,
                audio_format::AudioFormat format,
                BufferLength_t bufferLength,
                PeriodSize_t periodSize,
                const std::optional<audio_device::DeviceId>& inputDeviceId = std::nullopt,
                const std::optional<audio_device::ChannelCount_t>& numberOfInputChannels = std::nullopt,
                const std::optional<audio_device::DeviceId>& outputDeviceId = std::nullopt,
                const std::optional<audio_device::ChannelCount_t>& numberOfOutputChannels = std::nullopt) noexcept -> std::expected<std::shared_ptr<AudioStreamParams>, std::string>;

export [[nodiscard]] auto toString(const AudioStreamParams& audioStreamParams) -> std::string;
}