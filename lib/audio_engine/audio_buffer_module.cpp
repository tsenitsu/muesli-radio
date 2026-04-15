export module audio_buffer;

import std;
import audio_device;
import audio_stream_params;

namespace audio_engine::audio_buffer {

// TODO: Use mdspan when it will be iterable and support subspan
export template<class T> using AudioChannel = std::span<T>;

export template <typename T>
struct AudioBufferView {
    AudioBufferView(const AudioChannel<T>& left, const AudioChannel<T>& right)
      : m_leftMono { left },
        m_right { right } {}

    template <typename U> requires std::convertible_to<AudioChannel<U>, AudioChannel<T>>
    AudioBufferView(const AudioBufferView<U>& other)
      : m_leftMono { other.m_leftMono },
        m_right { other.m_right } {}

    AudioChannel<T> m_leftMono;
    AudioChannel<T> m_right;
};

export template <typename T> using ReadOnlyAudioBufferView = AudioBufferView<const T>;

// AudioBuffer contains non-interleaved data
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

    [[nodiscard]] auto view(const audio_device::ChannelCount_t leftChannel, const std::optional<audio_device::ChannelCount_t>& rightChannel = std::nullopt) const -> AudioBufferView<T> {
        AudioChannel<T> leftMono { AudioChannel<T> {} };
        AudioChannel<T> right { AudioChannel<T> {} };

        if (isChannelAllowed(leftChannel)) leftMono = m_channels[leftChannel];
        if (rightChannel.has_value() and isChannelAllowed(rightChannel.value())) right = m_channels[rightChannel.value()];

        return AudioBufferView<T> { leftMono, right };
    }

    auto resize(const audio_device::ChannelCount_t newChannelCount, const audio_stream_params::BufferLength_t newBufferLength) {
        if (newChannelCount == numberOfChannels() and newBufferLength == bufferLength()) {
            clear();
            return;
        }

        m_channels.clear();
        m_buffer.resize(newChannelCount * newBufferLength);

        buildChannels(newChannelCount, newBufferLength);
        clear();
    }

    auto addFrom(const AudioBuffer& bufferSrc, const  audio_device::ChannelCount_t channelSrc, const  audio_device::ChannelCount_t channelDest) -> void {
        if (not bufferSrc.isChannelAllowed(channelSrc) or not isChannelAllowed(channelDest))
            return;

        const auto maxLength { std::min(std::ranges::ssize(bufferSrc.m_channels[channelSrc]), std::ranges::ssize(m_channels[channelDest])) };

        std::ranges::transform(std::ranges::begin(bufferSrc.m_channels[channelSrc]), std::ranges::next(std::ranges::begin(bufferSrc.m_channels[channelSrc]), maxLength),
            std::ranges::begin(m_channels[channelDest]), std::ranges::next(std::ranges::begin(m_channels[channelDest]), maxLength), std::ranges::begin(m_channels[channelDest]), std::plus<T>());
    }

    auto copyFrom(const AudioBuffer& bufferSrc, const  audio_device::ChannelCount_t channelSrc, const audio_device::ChannelCount_t channelDest) -> void {
        if (not bufferSrc.isChannelAllowed(channelSrc) or not isChannelAllowed(channelDest))
            return;

        const auto maxLength { std::min(std::ranges::ssize(bufferSrc.m_channels[channelSrc]), std::ranges::ssize(m_channels[channelDest])) };

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

    auto copyFromRawBuffer(const T* bufferSrc, const audio_device::ChannelCount_t numberOfChannels, const audio_stream_params::BufferLength_t samplesPerChannel, const bool deinterleave = true, const audio_stream_params::BufferLength_t offset = 0) -> void {
        if (bufferSrc == nullptr or m_channels.size() == 0 or not isRawBufferCompatible(numberOfChannels, samplesPerChannel, offset))
            return;

        std::span rawBuffer { bufferSrc, numberOfChannels * samplesPerChannel };

        if (not deinterleave) {
            if (offset == 0) {
                std::ranges::copy(std::ranges::begin(rawBuffer), std::ranges::end(rawBuffer), std::ranges::begin(m_buffer));
                return;
            }

            for (auto currentChannelIdx { audio_device::ChannelCount_t { 0 } }; const auto channel: m_channels) {
                const auto srcBegin { std::next(std::ranges::begin(rawBuffer), currentChannelIdx * samplesPerChannel) };
                const auto srcEnd { std::next(srcBegin, samplesPerChannel) };

                const auto destBegin { std::next(std::ranges::begin(channel), offset) };
                std::ranges::copy(srcBegin, srcEnd, destBegin);

                ++currentChannelIdx;
            }

            return;
        }

        for (auto currentSample { static_cast<audio_stream_params::BufferLength_t>(0) }; currentSample < samplesPerChannel; ++currentSample) {
            for (auto currentChannel { static_cast<audio_device::ChannelCount_t>(0) }; currentChannel < numberOfChannels; ++currentChannel) {
                m_buffer[bufferLength() * currentChannel + currentSample + offset] = rawBuffer[currentSample * numberOfChannels + currentChannel];
            }
        }
    }

    auto writeToRawBuffer(T* bufferDest, const audio_device::ChannelCount_t numberOfChannels,
                      const audio_stream_params::BufferLength_t samplesPerChannel,
                      const bool interleave = true,
                      const audio_stream_params::BufferLength_t offset = 0) const -> void {
        if (bufferDest == nullptr or m_channels.size() == 0 or not isRawBufferCompatible(numberOfChannels, samplesPerChannel, offset))
            return;

        std::span rawBuffer { bufferDest, numberOfChannels * samplesPerChannel };

        if (not interleave) {
            if (offset == 0) {
                std::ranges::copy(std::ranges::begin(m_buffer), std::ranges::end(m_buffer), std::ranges::begin(rawBuffer));
                return;
            }

            for (auto currentChannelIdx { audio_device::ChannelCount_t { 0 } }; const auto& channel : m_channels) {
                const auto srcBegin { std::next(std::ranges::begin(channel), offset) };
                const auto srcEnd   { std::next(srcBegin, samplesPerChannel) };

                const auto destBegin { std::next(std::ranges::begin(rawBuffer), currentChannelIdx * samplesPerChannel) };
                std::ranges::copy(srcBegin, srcEnd, destBegin);

                ++currentChannelIdx;
            }

            return;
        }

        for (auto currentSample { static_cast<audio_stream_params::BufferLength_t>(0) }; currentSample < samplesPerChannel; ++currentSample) {
            for (auto currentChannel { static_cast<audio_device::ChannelCount_t>(0) }; currentChannel < numberOfChannels; ++currentChannel) {
                rawBuffer[currentSample * numberOfChannels + currentChannel] = m_buffer[bufferLength() * currentChannel + currentSample + offset];
            }
        }
    }

    [[nodiscard]] auto computeStats(const audio_device::ChannelCount_t channel) const -> std::tuple<T, T, float> {
        auto min { std::numeric_limits<T>::max() };
        // Do not use ::min() because when using floating point values, it gives the minimum positive
        auto max { std::numeric_limits<T>::lowest() };
        auto rms { float { 0.0f } };

        if (isChannelAllowed(channel) and bufferLength() != 0) {
            auto squareSum { 0.0 };

            for (const auto sample: m_channels[channel]) {
                min = std::min(min, sample);
                max = std::max(max, sample);

                const auto doubleSample { static_cast<double>(sample) };

                squareSum += doubleSample * doubleSample;
            }

            rms = static_cast<float>(std::sqrt(squareSum / bufferLength()));
        }

        return std::make_tuple(min, max, rms);
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

    [[nodiscard]] auto isRawBufferCompatible(const audio_device::ChannelCount_t numberOfChannels, const audio_stream_params::BufferLength_t samplesPerChannel, const audio_stream_params::BufferLength_t offset) const noexcept -> bool {
        return numberOfChannels == m_channels.size() and samplesPerChannel + offset <= m_channels[0].size();
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

export template <typename T> requires std::is_arithmetic_v<T>
class AudioStats final {
public:
    AudioStats (const T min, const T max, const float rms)
     :  m_min { min },
        m_max { max },
        m_rms { rms } {}

    auto min(const T min) -> void {
        return m_min.store(min, std::memory_order_relaxed);
    }

    auto max(const T max) -> void {
        return m_max.store(max, std::memory_order_relaxed);
    }

    auto rms(const float rms) -> void {
        return m_rms.store(rms, std::memory_order_relaxed);
    }

    [[nodiscard]] auto min() const -> T {
        return m_min.load(std::memory_order_relaxed);
    }

    [[nodiscard]] auto max() const -> T {
        return m_max.load(std::memory_order_relaxed);
    }

    [[nodiscard]] auto rms() const -> float {
        return m_rms.load(std::memory_order_relaxed);
    }

private:
    std::atomic<T> m_min;
    std::atomic<T> m_max;
    std::atomic<float> m_rms;
};


export template <typename T>
[[nodiscard]] auto makeAudioStats(const T min = T { 0 }, const T max = T { 0 }, const float rms = 0.0f) -> std::unique_ptr<AudioStats<T>> {
    return std::make_unique<AudioStats<T>>(min, max, rms);
}

}