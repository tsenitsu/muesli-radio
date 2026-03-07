export module audio_recorder;

import std;

import audio_writer;
import audio_device;
import channel_routing;
import audio_format;
import audio_buffer;

namespace audio_engine::audio_recorder {

export class AudioRecorder {
public:
    AudioRecorder(audio_device::SampleRate_t sampleRate, audio_format::AudioFormat format,
        const std::vector<std::string>& fileNames, const std::vector<audio_mixer::ChannelRouting>& routingList);

    virtual ~AudioRecorder() = default;

    [[nodiscard]] auto write(const audio_buffer::AudioBuffer<float>& audioBuffer) const -> bool;
private:
    std::vector<std::unique_ptr<AudioWriter>> m_writers;
    std::vector<audio_mixer::ChannelRouting> m_routing;
};

export [[nodiscard]] auto makeAudioRecorder(audio_device::SampleRate_t sampleRate, audio_format::AudioFormat format,
    const std::vector<std::string>& fileNames, const std::vector<audio_mixer::ChannelRouting>& routingList) -> std::expected<std::unique_ptr<AudioRecorder>, std::string> {

    try {
        return std::make_unique<AudioRecorder>(sampleRate, format, fileNames, routingList);
    } catch (const std::exception& e) {
        return std::unexpected { std::string { e.what()} };
    }
}

}