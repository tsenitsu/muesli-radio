module audio_recorder;

namespace audio_engine::audio_recorder {

AudioRecorder::AudioRecorder([[maybe_unused]]const audio_device::SampleRate_t sampleRate,[[maybe_unused]] const audio_format::AudioFormat format,
            [[maybe_unused]]const std::vector<std::string>& fileNames, [[maybe_unused]]const std::vector<audio_mixer::ChannelRouting>& routingList)
  : m_writers {},
    m_routing {} {

    if (fileNames.size() == 0) {
        throw std::invalid_argument( "No files provided");
    }

    auto sortedFileNames { fileNames };
    std::ranges::sort(sortedFileNames);

    if (std::ranges::adjacent_find(sortedFileNames) != sortedFileNames.end()) {
        throw std::invalid_argument("Duplicate file names provided");
    }

    if (routingList.size() == 0) {
        throw std::invalid_argument( "No routing list provided");
    }

    if (fileNames.size() != routingList.size()) {
        throw std::invalid_argument("Mismatch between number of files and number of channel routing");
    }

    if (format != audio_format::AudioFormat::SignedInt16 and format != audio_format::AudioFormat::SignedInt24 and format != audio_format::AudioFormat::Float32) {
        throw std::invalid_argument("Unsupported audio format");
    }

    for (const auto [name, routing]: std::ranges::views::zip(fileNames, routingList)) {
        const auto isMono { routing.isMono() };
        const auto isStereo { routing.isStereo() };

        if (not isMono and not isStereo) {
            continue;
        }

        const auto channelCount { isStereo? audio_device::ChannelCount_t { 2 } : audio_device::ChannelCount_t { 1 } };

        std::unique_ptr<AudioWriter> audioWriter { nullptr };

        switch (format) {
            case audio_format::AudioFormat::SignedInt16:
                if (auto result { audio_recorder::makeAudioWriter<audio_format::AudioFormat::SignedInt16>(
                    name, sampleRate, channelCount) }; not result.has_value()) {
                    throw std::runtime_error { std::move(result).error() };
                } else {
                    audioWriter = std::move(result).value();
                }

                break;

            case audio_format::AudioFormat::SignedInt24:
                if (auto result { audio_recorder::makeAudioWriter<audio_format::AudioFormat::SignedInt24>(
                    name, sampleRate, channelCount) }; not result.has_value()) {
                    throw std::runtime_error { std::move(result).error() };
                } else {
                    audioWriter = std::move(result).value();
                }

                break;
            case audio_format::AudioFormat::Float32:
                if (auto result { audio_recorder::makeAudioWriter<audio_format::AudioFormat::Float32>(
                    name, sampleRate, channelCount) }; not result.has_value()) {
                    throw std::runtime_error { std::move(result).error() };
                } else {
                    audioWriter = std::move(result).value();
                }

                break;
            default:
                std::unreachable();
        }

        m_writers.emplace_back(std::move(audioWriter));
        m_routing.push_back(routing);
    }

    // Should never happen
    if (m_writers.size() != m_routing.size()) {
        throw std::runtime_error { "Mismatch between audio writers and routing list" };
    }

    if (m_writers.size() == 0) {
        throw std::invalid_argument { "No audio writers provided" };
    }
}

auto AudioRecorder::write(const audio_buffer::AudioBuffer<float> &audioBuffer) const -> bool {
    auto writeResult { true };
    for (const auto [writer, routing]: std::ranges::views::zip(m_writers, m_routing)) {
        writeResult &= writer->write(audioBuffer.view(routing.m_leftMono.value(), routing.m_right));
    }

    return writeResult;
}

}
