export module audio_mixer;

import std;

import audio_channel;
import channel_routing;
import audio_device;
import audio_buffer;

namespace audio_engine::audio_mixer {

export template <typename T>
class AudioMixer {
public:
    AudioMixer(const audio_device::ChannelCount_t inputChannelCount, const audio_device::ChannelCount_t outputChannelCount)
     :  m_inputChannels {},
        m_outputChannels {} {
        std::ranges::generate_n(std::back_inserter(m_inputChannels), inputChannelCount, [] () { return makeMixerChannel<T>(); });
        std::ranges::generate_n(std::back_inserter(m_outputChannels), outputChannelCount, [] () { return makeMixerChannel<T>(); });
    }

    virtual ~AudioMixer() = default;

    auto inputGain(const T gain, const audio_device::ChannelCount_t channel) -> void { if (not mixerChannelExists(channel, m_inputChannels)) { return; } m_inputChannels[channel]->gain(gain); }
    [[nodiscard]] auto inputGain(const audio_device::ChannelCount_t channel) const -> T { return mixerChannelExists(channel, m_inputChannels)? m_inputChannels[channel]->gain() : T {}; }

    auto outputGain(const T gain, const audio_device::ChannelCount_t channel) -> void { if (not mixerChannelExists(channel, m_outputChannels)) { return; } m_outputChannels[channel]->gain(gain); }
    [[nodiscard]] auto outputGain(const audio_device::ChannelCount_t channel) const -> T { return mixerChannelExists(channel, m_outputChannels)? m_outputChannels[channel]->gain() : T {}; }

    // We assume routing has been validated upon construction
    auto inputRouting(const std::pair<ChannelRouting, ChannelRouting>& routing, const audio_device::ChannelCount_t channel) -> void { if (not mixerChannelExists(channel, m_inputChannels)) { return; } m_inputChannels[channel]->routing(routing); }
    [[nodiscard]] auto inputRouting(const audio_device::ChannelCount_t channel) const -> std::pair<ChannelRouting, ChannelRouting> { return mixerChannelExists(channel, m_inputChannels)? m_inputChannels[channel]->routing() : std::make_pair(ChannelRouting {}, ChannelRouting {}); }

    // outputRouting is always stereo
    auto outputRouting(const ChannelRouting& routing, const audio_device::ChannelCount_t channel) -> void { if (not mixerChannelExists(channel, m_outputChannels) or not routing.isStereo()) { return; } m_outputChannels[channel]->routing(std::make_pair(ChannelRouting {}, routing)); }
    [[nodiscard]] auto outputRouting(const audio_device::ChannelCount_t channel) const -> ChannelRouting { return mixerChannelExists(channel, m_outputChannels)? m_outputChannels[channel]->routing().second : ChannelRouting {}; }

    auto processInput(const audio_buffer::ReadOnlyAudioBufferView<T>& input, const audio_buffer::AudioBufferView<T>& processed, const audio_device::ChannelCount_t channel) {
        if (not mixerChannelExists(channel, m_inputChannels)) { return; } m_inputChannels[channel]->process(input, processed);
    }

    auto processOutput(const audio_buffer::ReadOnlyAudioBufferView<T>& input, const audio_buffer::AudioBufferView<T>& processed, const audio_device::ChannelCount_t channel) {
        if (not mixerChannelExists(channel, m_outputChannels)) { return; } m_outputChannels[channel]->process(input, processed);
    }

    static auto mix(const audio_buffer::ReadOnlyAudioBufferView<T>& input, const audio_buffer::AudioBufferView<T>& output) -> void {
        const bool inputIsStereo { not input.m_right.empty() };
        const bool outputIsStereo { not output.m_right.empty() };

        // Both stereo
        if (inputIsStereo and outputIsStereo) {
            std::ranges::transform(input.m_leftMono, output.m_leftMono, output.m_leftMono.begin(), std::plus {});
            std::ranges::transform(input.m_right, output.m_right, output.m_right.begin(), std::plus {});
        // Stereo input and mono output
        } else if (inputIsStereo and not outputIsStereo) {
            std::ranges::transform(input.m_leftMono, output.m_leftMono, output.m_leftMono.begin(), std::plus {});
            std::ranges::transform(input.m_right, output.m_leftMono, output.m_leftMono.begin(), std::plus {});

            std::ranges::transform(output.m_leftMono, output.m_leftMono.begin(), [] (T sample) { return sample / T { 2 }; });
        // Mono input and stereo output
        } else if (not inputIsStereo and outputIsStereo) {
            std::ranges::transform(input.m_leftMono, output.m_leftMono, output.m_leftMono.begin(), std::plus {});
            std::ranges::transform(input.m_leftMono, output.m_right, output.m_right.begin(), std::plus {});
        // Both mono
        } else {
            std::ranges::transform(input.m_leftMono, output.m_leftMono, output.m_leftMono.begin(), std::plus {});
        }
    }

protected:
    [[nodiscard]] auto mixerChannelExists(const audio_device::ChannelCount_t channel, const std::vector<std::unique_ptr<MixerChannel<T>>>& channelList) const -> bool {
        return channel < std::ranges::size(channelList)? true : false;
    }

    std::vector<std::unique_ptr<MixerChannel<T>>> m_inputChannels;
    std::vector<std::unique_ptr<MixerChannel<T>>> m_outputChannels;
};

export template <typename T>
[[nodiscard]] auto makeAudioMixer(const audio_device::ChannelCount_t inputChannelCount, const audio_device::ChannelCount_t outputChannelCount) -> std::expected<std::unique_ptr<AudioMixer<T>>, std::string> {
    if (inputChannelCount == 0 and outputChannelCount == 0) {
        return std::unexpected { std::string { "Mixer must have at least one input or output channel" } };
    }

    return std::make_unique<AudioMixer<T>>(inputChannelCount, outputChannelCount);
}

}