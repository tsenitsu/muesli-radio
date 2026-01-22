export module audio_channel;

import std;

import channel_routing;
import audio_buffer;

namespace audio_engine::audio_mixer {

export template <typename T> requires std::atomic<T>::is_always_lock_free and std::atomic<SerializedInputOutputRouting_t>::is_always_lock_free
class MixerChannel final {
public:
    MixerChannel(const T gain, const std::pair<ChannelRouting, ChannelRouting>& routing)
      : m_gain { gain },
        m_routing { ChannelRoutingSerializer::serializeInputOutput(routing.first, routing.second) } {}

    auto gain(const T gain) -> void { m_gain.store(gain, std::memory_order_relaxed); }
    [[nodiscard]] auto gain() const -> T { return m_gain.load(std::memory_order_relaxed); }

    auto routing(const std::pair<ChannelRouting, ChannelRouting>& routing) -> void {
        // We assume ChannelRouting has been validated upon construction
        m_routing.store(ChannelRoutingSerializer::serializeInputOutput(routing.first, routing.second), std::memory_order_relaxed);
    }
    [[nodiscard]] auto routing() const -> std::pair<ChannelRouting, ChannelRouting> {
        return ChannelRoutingSerializer::deserializeInputOutput(m_routing.load(std::memory_order_relaxed));
    }

    // We assume input and processed have the same buffer length
    auto process(const audio_buffer::ReadOnlyAudioBufferView<T>& input, const audio_buffer::AudioBufferView<T>& processed) -> void {
        auto gain { m_gain.load(std::memory_order_relaxed) };

        if (not input.m_leftMono.empty()) {
            std::ranges::transform(input.m_leftMono, processed.m_leftMono.begin(), [&gain] (T sample) { return gain * sample; });
        }

        if (not input.m_right.empty()) {
            std::ranges::transform(input.m_right, processed.m_right.begin(), [&gain] (T sample) { return gain * sample; });
        }
    }

private:
    std::atomic<T> m_gain;
    std::atomic<SerializedInputOutputRouting_t> m_routing;
};

export template<typename T>
[[nodiscard]] auto makeMixerChannel(const T gain = T { 1 }, const std::pair<ChannelRouting, ChannelRouting>& routing = std::make_pair(*makeChannelRouting().value(), *makeChannelRouting().value())) -> std::unique_ptr<MixerChannel<T>> {
    return std::make_unique<MixerChannel<T>>(gain, routing);
}

}