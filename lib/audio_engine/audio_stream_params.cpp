module audio_stream_params;

namespace audio_engine::audio_stream_params {

AudioStreamParams::AudioStreamParams(const audio_device::SampleRate_t sampleRate,
                        const audio_format::AudioFormat format,
                        const BufferLength_t bufferLength,
                        const PeriodSize_t periodSize)
 :  m_sampleRate { sampleRate },
    m_format { format },
    m_bufferLength { bufferLength },
    m_periodSize { periodSize }
{}

InputAudioStreamParams::InputAudioStreamParams(const audio_device::SampleRate_t sampleRate,
                const audio_format::AudioFormat format,
                const BufferLength_t bufferLength,
                const PeriodSize_t periodSize,
                const audio_device::DeviceId& inputDeviceId,
                const audio_device::ChannelCount_t numberOfInputChannels)
 :  AudioStreamParams { sampleRate, format, bufferLength, periodSize },
    m_inputDeviceId { inputDeviceId },
    m_numberOfInputChannels { numberOfInputChannels }
{}

InputAudioStreamParams::InputAudioStreamParams(const audio_device::DeviceId& inputDeviceId,
                const audio_device::ChannelCount_t numberOfInputChannels)
 :  m_inputDeviceId { inputDeviceId },
    m_numberOfInputChannels { numberOfInputChannels }
{}

OutputAudioStreamParams::OutputAudioStreamParams(const audio_device::SampleRate_t sampleRate,
                    const audio_format::AudioFormat format,
                    const BufferLength_t bufferLength,
                    const PeriodSize_t periodSize,
                    const audio_device::DeviceId& outputDeviceId,
                    const audio_device::ChannelCount_t numberOfOutputChannels)
 :  AudioStreamParams { sampleRate, format, bufferLength, periodSize },
    m_outputDeviceId { outputDeviceId },
    m_numberOfOutputChannels { numberOfOutputChannels }
{}

OutputAudioStreamParams::OutputAudioStreamParams(const audio_device::DeviceId& outputDeviceId,
                    const audio_device::ChannelCount_t numberOfOutputChannels)
 :  m_outputDeviceId { outputDeviceId },
    m_numberOfOutputChannels { numberOfOutputChannels }
{}

DuplexAudioStreamParams::DuplexAudioStreamParams(const audio_device::SampleRate_t sampleRate,
                    const audio_format::AudioFormat format,
                    const BufferLength_t bufferLength,
                    const PeriodSize_t periodSize,
                    const audio_device::DeviceId& inputDeviceId,
                    const audio_device::ChannelCount_t numberOfInputChannels,
                    const audio_device::DeviceId& outputDeviceId,
                    const audio_device::ChannelCount_t numberOfOutputChannels)
 :  AudioStreamParams { sampleRate, format, bufferLength, periodSize },
    InputAudioStreamParams { inputDeviceId, numberOfInputChannels },
    OutputAudioStreamParams { outputDeviceId, numberOfOutputChannels }
{}


auto makeAudioStreamParams(const audio_device::SampleRate_t sampleRate,
                    const audio_format::AudioFormat format,
                    const BufferLength_t bufferLength,
                    const PeriodSize_t periodSize,
                    const std::optional<audio_device::DeviceId>& inputDeviceId,
                    const std::optional<audio_device::ChannelCount_t>& numberOfInputChannels,
                    const std::optional<audio_device::DeviceId>& outputDeviceId,
                    const std::optional<audio_device::ChannelCount_t>& numberOfOutputChannels) noexcept -> std::expected<std::shared_ptr<AudioStreamParams>, std::string> {

    if (sampleRate < 44100)
        return std::unexpected { std::string { "Invalid sample rate" } };

    if (format != audio_format::AudioFormat::Float32)
        return std::unexpected { std::string { "Invalid format" } };

    if (bufferLength < 32)
        return std::unexpected { std::string { "Invalid buffer length" } };

    if (periodSize < 3)
        return std::unexpected { std::string { "Invalid period size" } };

    auto inputDeviceProvided { false };
    auto outputDeviceProvided { false };

    if (inputDeviceId.has_value()) {
        if (not numberOfInputChannels.has_value())
           return std::unexpected { std::string { "Number of input channels not set" } };

        if (numberOfInputChannels.value() == 0)
            return std::unexpected { std::string { "Invalid number of input channels" } };

        inputDeviceProvided = true;
    }

    if (outputDeviceId.has_value()) {
        if (not numberOfOutputChannels.has_value())
            return std::unexpected { std::string { "Number of output channels not set" } };

        if (numberOfOutputChannels.value() == 0)
            return std::unexpected { std::string { "Invalid number of output channels" } };

        outputDeviceProvided = true;
    }

    if (inputDeviceProvided and outputDeviceProvided)
        return std::make_shared<DuplexAudioStreamParams>(sampleRate, format, bufferLength, periodSize,
                                                    inputDeviceId.value(), numberOfInputChannels.value(),
                                                    outputDeviceId.value(), numberOfOutputChannels.value());

    if (inputDeviceProvided)
        return std::make_shared<InputAudioStreamParams>(sampleRate, format, bufferLength, periodSize,
                                                inputDeviceId.value(), numberOfInputChannels.value());

    if (outputDeviceProvided)
        return std::make_shared<OutputAudioStreamParams>(sampleRate, format, bufferLength, periodSize,
                                                outputDeviceId.value(), numberOfOutputChannels.value());

    return std::unexpected { std::string { "No devices provided" } };
}

auto toString(const AudioStreamParams& audioStreamParams) -> std::string {
    auto params { std::format("Sample rate: {}\n", audioStreamParams.m_sampleRate) };

    params.append(std::format("Format: {}", audio_format::toString(audioStreamParams.m_format).value()));
    params.append(std::format("\nBuffer length: {}\n", audioStreamParams.m_bufferLength));
    params.append(std::format("\nPeriod size: {}\n", audioStreamParams.m_periodSize));

    try {
        const auto& inputParams { dynamic_cast<const InputAudioStreamParams&>(audioStreamParams) };
        params.append(std::format("Number of input channels: {}\n", inputParams.m_numberOfInputChannels));
    } catch ([[maybe_unused]] const std::bad_cast&) {}

    try {
        const auto& outputParams { dynamic_cast<const OutputAudioStreamParams&>(audioStreamParams) };
        params.append(std::format("Number of output channels: {}\n", outputParams.m_numberOfOutputChannels));
    } catch ([[maybe_unused]] const std::bad_cast&) {}

    return params;
}

}