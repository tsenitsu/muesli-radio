module;
#include <miniaudio.h>
export module ring_audio_buffer;

import std;

import audio_device;
import audio_stream_params;
import audio_buffer;

namespace audio_engine::ring_audio_buffer {

// RingAudioBuffer is always interleaved
export template <typename T> requires std::is_arithmetic_v<T> and (not std::same_as<T, bool>)
class RingAudioBuffer final {
public:
    RingAudioBuffer(const audio_device::ChannelCount_t channelCount, const audio_stream_params::BufferLength_t bufferLength)
      : m_rb {}
    {
        constexpr auto format = getAudioFormat();
        const auto maFormat = audio_format::toMaFormat(format).value();

        if (ma_pcm_rb_init(maFormat, channelCount, bufferLength, nullptr, nullptr, &m_rb) != MA_SUCCESS) {
            throw std::runtime_error("Unable to initialize ring buffer");
        }
    }

    ~RingAudioBuffer() {
        ma_pcm_rb_uninit(&m_rb);
    }

    RingAudioBuffer(const RingAudioBuffer&) = delete;
    RingAudioBuffer& operator=(const RingAudioBuffer&) = delete;
    RingAudioBuffer(RingAudioBuffer&&) = delete;
    RingAudioBuffer& operator=(RingAudioBuffer&&) = delete;

    template <typename G> requires std::same_as<T, G>
    [[nodiscard]] auto enqueue(const audio_buffer::AudioBuffer<G>& buffer) -> bool {
        if (not isAudioBufferCompatible(buffer.numberOfChannels())) {
            return false;
        }

        void* writePtr { nullptr };
        auto writeFrames { buffer.bufferLength() };

        if (auto result { ma_pcm_rb_acquire_write(&m_rb, &writeFrames, &writePtr) }; result != MA_SUCCESS or writeFrames < buffer.bufferLength()) {
            return false;
        }

        buffer.writeToRawBuffer(static_cast<G*>(writePtr), buffer.numberOfChannels(), writeFrames, true);

        return ma_pcm_rb_commit_write(&m_rb, writeFrames) == MA_SUCCESS;
    }

    template <typename G> requires std::same_as<T, G>
    [[nodiscard]] auto dequeue(audio_buffer::AudioBuffer<G>& buffer) -> bool {
        const auto totalFramesToRead { ma_pcm_rb_available_read(&m_rb) };

        if (totalFramesToRead == 0) {
            return true;
        }

        buffer.resize(ma_pcm_rb_get_channels(&m_rb), totalFramesToRead);

        void* readPtr { nullptr };
        auto readFrames { totalFramesToRead };

        if (ma_pcm_rb_acquire_read(&m_rb, &readFrames, &readPtr) != MA_SUCCESS) {
            return false;
        }

        buffer.copyFromRawBuffer(static_cast<const G*>(readPtr), buffer.numberOfChannels(), readFrames, true, 0);

        if (ma_pcm_rb_commit_read(&m_rb, readFrames) != MA_SUCCESS) {
            return false;
        }

        const auto remainingFramesToRead { totalFramesToRead - readFrames };
        if (remainingFramesToRead == 0)
            return true;

        auto remainingFrames { remainingFramesToRead };
        if (ma_pcm_rb_acquire_read(&m_rb, &remainingFrames, &readPtr) != MA_SUCCESS or remainingFrames < remainingFramesToRead)
            return false;

        buffer.copyFromRawBuffer(static_cast<const G*>(readPtr), buffer.numberOfChannels(), remainingFrames, true, readFrames);

        return ma_pcm_rb_commit_read(&m_rb, remainingFrames) == MA_SUCCESS;
    }

protected:
    [[nodiscard]] static constexpr auto getAudioFormat() -> audio_format::AudioFormat {
        if constexpr (std::is_integral_v<T>) {
            if constexpr (std::is_unsigned_v<T>) {
                if constexpr (sizeof(T) == 1) {
                    return audio_format::AudioFormat::UnsignedInt8;
                } else {
                    static_assert(sizeof(T) == 1, "Only 8-bit unsigned integers supported");
                }
            } else {
                if constexpr (sizeof(T) == 2) {
                    return audio_format::AudioFormat::SignedInt16;
                } else if constexpr (sizeof(T) == 3) {
                    return audio_format::AudioFormat::SignedInt24;
                } else if constexpr (sizeof(T) == 4) {
                    return audio_format::AudioFormat::SignedInt32;
                } else {
                    static_assert(sizeof(T) == 2 or sizeof(T) == 3 or sizeof(T) == 4, "8-bit signed integers are not supported");
                }
            }
        } else {
            if constexpr (sizeof(T) == 4) {
                return audio_format::AudioFormat::Float32;
            } else {
                static_assert(sizeof(T) == 4, "Only 32-bit floats supported");
            }
        }

        std::unreachable();
    }

    [[nodiscard]] auto isAudioBufferCompatible(const audio_device::ChannelCount_t numberOfChannels) const -> bool {
        return ma_pcm_rb_get_channels(&m_rb) == numberOfChannels;
    }

private:
    ma_pcm_rb m_rb;
};

export template <typename T>
[[nodiscard]] auto makeRingAudioBuffer(const audio_device::ChannelCount_t channelCount, const audio_stream_params::BufferLength_t bufferLength) -> std::expected<std::unique_ptr<RingAudioBuffer<T>>, std::string> {
    try {
        return std::make_unique<RingAudioBuffer<T>>(channelCount, bufferLength);
    } catch (const std::exception& ex) {
        return std::unexpected { std::string { ex.what() } };
    }
}

}