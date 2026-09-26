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

DeviceSelection::DeviceSelection(audio_device::DeviceId deviceId,
                                 const audio_device::ChannelCount_t channelCount)
 :  m_deviceId { std::move(deviceId) },
    m_channelCount { channelCount }
{}

LoopbackAudioStreamParams::LoopbackAudioStreamParams(const audio_device::SampleRate_t sampleRate,
                    const audio_format::AudioFormat format,
                    const BufferLength_t bufferLength,
                    const PeriodSize_t periodSize,
                    const audio_device::DeviceId& loopbackDeviceId,
                    const audio_device::ChannelCount_t numberOfLoopbackChannels)
 :  AudioStreamParams { sampleRate, format, bufferLength, periodSize },
    m_loopbackDeviceId { loopbackDeviceId },
    m_numberOfLoopbackChannels { numberOfLoopbackChannels }
{}

auto makeAudioStreamParams(const audio_device::SampleRate_t sampleRate,
                    const audio_format::AudioFormat format,
                    const BufferLength_t bufferLength,
                    const PeriodSize_t periodSize,
                    const std::optional<DeviceSelection>& input,
                    const std::optional<DeviceSelection>& output,
                    const std::optional<DeviceSelection>& loopback) -> std::expected<std::unique_ptr<AudioStreamParams>, std::string> {

    if (sampleRate < 44100)
        return std::unexpected { std::string { "Invalid sample rate" } };

    if (format != audio_format::AudioFormat::Float32)
        return std::unexpected { std::string { "Invalid format" } };

    if (bufferLength < 32)
        return std::unexpected { std::string { "Invalid buffer length" } };

    if (periodSize < 3)
        return std::unexpected { std::string { "Invalid period size" } };

    // Loopback captures a playback device and needs its own ma_device,
    // so it can't be combined with input or output.
    if (loopback.has_value()) {
        if (input.has_value() or output.has_value())
            return std::unexpected { std::string { "Loopback cannot be combined with input or output" } };

        if (loopback->m_channelCount == 0)
            return std::unexpected { std::string { "Invalid number of loopback channels" } };

        return std::make_unique<LoopbackAudioStreamParams>(sampleRate, format, bufferLength, periodSize,
                                                    loopback->m_deviceId, loopback->m_channelCount);
    }

    if (input.has_value() and input->m_channelCount == 0)
        return std::unexpected { std::string { "Invalid number of input channels" } };

    if (output.has_value() and output->m_channelCount == 0)
        return std::unexpected { std::string { "Invalid number of output channels" } };

    if (input.has_value() and output.has_value())
        return std::make_unique<DuplexAudioStreamParams>(sampleRate, format, bufferLength, periodSize,
                                                    input->m_deviceId, input->m_channelCount,
                                                    output->m_deviceId, output->m_channelCount);

    if (input.has_value())
        return std::make_unique<InputAudioStreamParams>(sampleRate, format, bufferLength, periodSize,
                                                input->m_deviceId, input->m_channelCount);

    if (output.has_value())
        return std::make_unique<OutputAudioStreamParams>(sampleRate, format, bufferLength, periodSize,
                                                output->m_deviceId, output->m_channelCount);

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

    try {
        const auto& loopbackParams { dynamic_cast<const LoopbackAudioStreamParams&>(audioStreamParams) };
        params.append(std::format("Number of loopback channels: {}\n", loopbackParams.m_numberOfLoopbackChannels));
    } catch ([[maybe_unused]] const std::bad_cast&) {}

    return params;
}

}