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
      : m_rb {},
        m_channels { channelCount }
    {
        constexpr auto format = getAudioFormat();
        const auto maFormat = audio_format::toMaFormat(format).value();

        ma_audio_ring_buffer_config bufferConfig { ma_audio_ring_buffer_config_init(maFormat, channelCount, 0, bufferLength) };
        bufferConfig.pBuffer = nullptr;
        bufferConfig.pAllocationCallbacks = nullptr;

        if (ma_audio_ring_buffer_init(&bufferConfig, &m_rb) != MA_SUCCESS) {
            throw std::runtime_error("Unable to initialize ring buffer");
        }
    }

    ~RingAudioBuffer() {
        ma_audio_ring_buffer_uninit(&m_rb);
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

        const auto totalFrames { buffer.bufferLength() };
        auto remaining { totalFrames };
        auto offset { audio_stream_params::BufferLength_t { 0 } };  // offset in the source buffer

        while (remaining > 0) {
            void* writePtr { nullptr };
            const auto framesToWrite { remaining };

            // Try to map the next contiguous block
            auto framesMapped { ma_audio_ring_buffer_map_produce(&m_rb, framesToWrite, &writePtr) };
            if (framesMapped == 0) {
                // No space left – we might have partially written data already.
                // Since we commit immediately after writing, we cannot roll back.
                // For simplicity, return false; caller should retry.
                return false;
            }

            // Write the mapped frames
            buffer.writeToRawBuffer(static_cast<G*>(writePtr), buffer.numberOfChannels(),
                                    framesMapped, true, offset);

            // Commit the write
            ma_audio_ring_buffer_unmap_produce(&m_rb, framesMapped);

            remaining -= framesMapped;
            offset    += framesMapped;
        }

        return true;
    }

    template <typename G> requires std::same_as<T, G>
    [[nodiscard]] auto dequeue(audio_buffer::AudioBuffer<G>& buffer) -> bool {
        auto availableFrames { audio_stream_params::BufferLength_t { 0 } };

        if (ma_audio_ring_buffer_get_length_in_pcm_frames(&m_rb, &availableFrames) != MA_SUCCESS) {
            return false;
        }

        // Pre‑size the output buffer
        buffer.resize(m_channels, availableFrames);

        auto remaining { availableFrames };
        auto offset { audio_stream_params::BufferLength_t { 0 } };

        while (remaining > 0) {
            void* readPtr { nullptr };
            const auto framesToRead { remaining };

            auto framesMapped { ma_audio_ring_buffer_map_consume(&m_rb, framesToRead, &readPtr) };
            if (framesMapped == 0) {
                return false;  // unexpected – we already know there is data
            }

            buffer.copyFromRawBuffer(static_cast<const G*>(readPtr), buffer.numberOfChannels(),
                                     framesMapped, true, offset);

            ma_audio_ring_buffer_unmap_consume(&m_rb, framesMapped);

            remaining -= framesMapped;
            offset    += framesMapped;
        }

        return true;
    }

    auto reset() -> void {
        ma_uint32 available {};

        while (ma_audio_ring_buffer_get_length_in_pcm_frames(&m_rb, &available) == MA_SUCCESS && available > 0) {
            void* discardPtr { nullptr };

            const ma_uint32 framesToDiscard { available };
            const ma_uint32 framesMapped { ma_audio_ring_buffer_map_consume(&m_rb, framesToDiscard, &discardPtr) };

            if (framesMapped == 0) {
                break; // should not happen if available > 0
            }

            ma_audio_ring_buffer_unmap_consume(&m_rb, framesMapped);
        }
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
        return m_channels == numberOfChannels;
    }

private:
    ma_audio_ring_buffer m_rb;
    audio_device::ChannelCount_t m_channels;
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