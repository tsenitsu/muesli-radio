export module audio_buffer;

import std;
import audio_device;
import audio_stream_params;

namespace audio_engine::audio_buffer {

// TODO: Use mdspan when it will be iterable and support subspan
export template<class T> using AudioChannel = std::span<T>;

export template <class T>
class AudioBuffer final {
public:
    AudioBuffer(const audio_device::ChannelCount_t numberOfChannels,
                const audio_stream_params::BufferLength_t  samplesPerChannel)
     :  m_channels {},
        m_buffer (numberOfChannels * samplesPerChannel, 0) {
        buildChannels(numberOfChannels, samplesPerChannel);
    }

    AudioBuffer(AudioBuffer&& other) noexcept
     :  m_channels { std::move(other.m_channels) },
        m_buffer { std::move(other.m_buffer) } {}

    [[nodiscard]] auto numberOfChannels() const noexcept -> audio_device::ChannelCount_t {
        return static_cast<audio_device::ChannelCount_t>(m_channels.size());
    }

    [[nodiscard]] auto bufferLength() const noexcept -> audio_stream_params::BufferLength_t {
        if (m_channels.size() == 0)
            return 0;

        return static_cast<audio_stream_params::BufferLength_t>(m_channels[0].size());
    }

    auto addFrom(const AudioBuffer& bufferSrc, const  audio_device::ChannelCount_t channelSrc, const  audio_device::ChannelCount_t channelDest) -> void {
        if (not bufferSrc.isChannelAllowed(channelSrc) or not isChannelAllowed(channelDest))
            return;

        const auto maxLength { std::min(bufferSrc.m_channels[channelSrc].size(), m_channels[channelDest].size()) };

        std::ranges::transform(std::ranges::begin(bufferSrc.m_channels[channelSrc]), std::ranges::next(std::ranges::begin(bufferSrc.m_channels[channelSrc]), maxLength),
            std::ranges::begin(m_channels[channelDest]), std::ranges::next(std::ranges::begin(m_channels[channelDest]), maxLength), std::ranges::begin(m_channels[channelDest]), std::plus<T>());
    }

    auto copyFrom(const AudioBuffer& bufferSrc, const  audio_device::ChannelCount_t channelSrc, const audio_device::ChannelCount_t channelDest) -> void {
        if (not bufferSrc.isChannelAllowed(channelSrc) or not isChannelAllowed(channelDest))
            return;

        const auto maxLength { std::min(bufferSrc.m_channels[channelSrc].size(), m_channels[channelDest].size()) };

        std::ranges::copy(std::ranges::begin(bufferSrc.m_channels[channelSrc]), std::ranges::next(std::ranges::begin(bufferSrc.m_channels[channelSrc]), maxLength), std::ranges::begin(m_channels[channelDest]));
    }

    auto clear(const std::optional<audio_device::ChannelCount_t> channelToClear = std::nullopt) -> void {
        if (channelToClear.has_value() and not isChannelAllowed(channelToClear.value()))
            return;

        if (not channelToClear.has_value()) {
            for (const auto& channel : m_channels)
                std::ranges::fill(std::ranges::begin(channel), std::ranges::end(channel), static_cast<T>(0));

            return;
        }

        std::ranges::fill(std::ranges::begin(m_channels[channelToClear.value()]), std::ranges::end(m_channels[channelToClear.value()]), static_cast<T>(0));
    }

    auto copyFromRawBuffer(const T* bufferSrc, const audio_device::ChannelCount_t numberOfChannels, const audio_stream_params::BufferLength_t samplesPerChannel, const bool deinterleave = true) -> void {
        if (bufferSrc == nullptr or m_channels.size() == 0 or not isRawBufferCompatible(numberOfChannels, samplesPerChannel))
            return;

        std::span rawBuffer { bufferSrc, numberOfChannels * samplesPerChannel };

        if (not deinterleave) {
            std::ranges::copy(std::ranges::begin(rawBuffer), std::ranges::end(rawBuffer), std::ranges::begin(m_buffer));
            return;
        }

        for (auto currentSample { static_cast<audio_stream_params::BufferLength_t>(0) }; currentSample < samplesPerChannel; ++currentSample) {
            for (auto currentChannel { static_cast<audio_device::ChannelCount_t>(0) }; currentChannel < numberOfChannels; ++currentChannel) {
                m_buffer[samplesPerChannel * currentChannel + currentSample] = rawBuffer[currentSample * numberOfChannels + currentChannel];
            }
        }
    }

    auto writeToRawBuffer(T* bufferDest, const audio_device::ChannelCount_t numberOfChannels, const audio_stream_params::BufferLength_t samplesPerChannel, const bool interleave = true) -> void {
        if (bufferDest == nullptr or m_channels.size() == 0 or not isRawBufferCompatible(numberOfChannels, samplesPerChannel))
            return;

        std::span rawBuffer { bufferDest, numberOfChannels * samplesPerChannel };

        if (not interleave) {
            std::ranges::copy(std::ranges::begin(m_buffer), std::ranges::end(m_buffer), std::ranges::begin(rawBuffer));
            return;
        }

        for (auto currentSample { static_cast<audio_stream_params::BufferLength_t>(0) }; currentSample < samplesPerChannel; ++currentSample) {
            for (auto currentChannel { static_cast<audio_device::ChannelCount_t>(0) }; currentChannel < numberOfChannels; ++currentChannel) {
                rawBuffer[currentSample * numberOfChannels + currentChannel] = m_buffer[samplesPerChannel * currentChannel + currentSample];
            }
        }
    }

    AudioBuffer<T>& operator= (const AudioBuffer<T>& otherBuffer) {
        if (this == &otherBuffer)
            return *this;

        for ( auto i { static_cast<audio_device::ChannelCount_t>(0) }; i < m_channels.size(); ++i) {
            this->copyFrom(otherBuffer, i, i);
        }

        return *this;
    }

    AudioBuffer<T>& operator+= (const AudioBuffer<T>& otherBuffer) {
        if (this == &otherBuffer)
            return *this;

        for ( auto i { static_cast<audio_device::ChannelCount_t>(0) }; i < m_channels.size(); ++i) {
            this->addFrom(otherBuffer, i, i);
        }

        return *this;
    }

    AudioBuffer<T>& operator+ (const AudioBuffer<T>& otherBuffer) = delete;

protected:
    void buildChannels(const audio_device::ChannelCount_t numberOfChannels,
                const audio_stream_params::BufferLength_t  samplesPerChannel) {

        for (auto i { static_cast<unsigned int>(0) }; i < numberOfChannels; ++i) {
            m_channels.emplace_back(std::span { m_buffer.data() + (samplesPerChannel * i), samplesPerChannel });
        }
    }
    [[nodiscard]] auto isChannelAllowed(const audio_device::ChannelCount_t channel) const noexcept -> bool {
        return channel < m_channels.size();
    }

    [[nodiscard]] auto isRawBufferCompatible(const audio_device::ChannelCount_t numberOfChannels, const audio_stream_params::BufferLength_t samplesPerChannel) const noexcept -> bool {
        return numberOfChannels == m_channels.size() and samplesPerChannel == m_channels[0].size();
    }

private:
    std::vector<AudioChannel<T>> m_channels;
    std::vector<T> m_buffer;
};

export template<typename T>
auto makeAudioBuffer(const audio_device::ChannelCount_t numberOfChannels,
                    const audio_stream_params::BufferLength_t  samplesPerChannel) -> std::unique_ptr<AudioBuffer<T>> {

    return std::make_unique<AudioBuffer<T>>(numberOfChannels, samplesPerChannel);
}

}