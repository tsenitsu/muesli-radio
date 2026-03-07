module;
#include <miniaudio.h>
export module audio_writer;

import std;

import audio_device;
import audio_format;
import audio_buffer;

namespace audio_engine::audio_recorder {

template <audio_format::AudioFormat format> requires (format == audio_format::AudioFormat::SignedInt16) or
    (format == audio_format::AudioFormat::SignedInt24) or (format == audio_format::AudioFormat::Float32)
struct SelectMaFormat;

template <>
struct SelectMaFormat<audio_format::AudioFormat::SignedInt16> {
    using type = ma_int16;
};

template <>
struct SelectMaFormat<audio_format::AudioFormat::SignedInt24> {
    using type = ma_uint8;
};

template <>
struct SelectMaFormat<audio_format::AudioFormat::Float32> {
    using type = ma_float;
};

export class AudioWriter {
public:
    virtual ~AudioWriter() = default;

    [[nodiscard]] virtual auto write(const audio_buffer::ReadOnlyAudioBufferView<float>& buffer) -> bool = 0;
};

template <audio_format::AudioFormat format>
class AudioWriterWithFormat: public AudioWriter {
public:
    AudioWriterWithFormat(std::string_view fileName, const audio_device::SampleRate_t sampleRate, const audio_device::ChannelCount_t channelCount)
      : m_encoderConfig {},
        m_encoder {},
        m_interleavedSamples {},
        m_convertedSamples {}
    {
        static_assert(format == audio_format::AudioFormat::SignedInt16 or format == audio_format::AudioFormat::SignedInt24 or format == audio_format::AudioFormat::Float32,
            "Unsupported output sample format");

        m_encoderConfig = ma_encoder_config_init(ma_encoding_format_wav, audio_format::toMaFormat(format).value(), channelCount, sampleRate);

        const auto fileNameWithExtension { std::string { fileName }.append(".wav") };

        if (ma_encoder_init_file(fileNameWithExtension.c_str(), &m_encoderConfig, &m_encoder) != MA_SUCCESS) {
            throw std::runtime_error("Failed to initialize output file");
        }
    }

    ~AudioWriterWithFormat() override {
        ma_encoder_uninit(&m_encoder);
    }

    [[nodiscard]] auto write(const audio_buffer::ReadOnlyAudioBufferView<float>& buffer) -> bool override {
        const auto isMono { not buffer.m_leftMono.empty() and buffer.m_right.empty() };
        const auto isStereo { not buffer.m_leftMono.empty() and not buffer.m_right.empty() };

        if (not isMono and not isStereo) {
            return false;
        }

        if ((isMono and m_encoderConfig.channels != 1) or (isStereo and m_encoderConfig.channels != 2)) {
            return false;
        }

        if (isStereo and buffer.m_leftMono.size() != buffer.m_right.size()) {
            return false;
        }

        const auto samplesPerChannel { buffer.m_leftMono.size() };
        const auto bufferLength { isStereo ? samplesPerChannel * 2 : samplesPerChannel };

        m_interleavedSamples.resize(bufferLength);
        std::ranges::fill(m_interleavedSamples, 0.0f);

        if (isMono) {
            std::ranges::copy(std::ranges::begin(buffer.m_leftMono), std::ranges::end(buffer.m_leftMono), std::ranges::begin(m_interleavedSamples));
        } else {
            for (unsigned int i { 0 }; i < samplesPerChannel; ++i) {
                m_interleavedSamples[i * 2] = buffer.m_leftMono[i];
                m_interleavedSamples[i * 2 + 1] = buffer.m_right[i];
            }
        }

        if constexpr (format == audio_format::AudioFormat::SignedInt16) {
            static_assert(std::same_as<typename decltype(m_convertedSamples)::value_type, ma_int16>, "Format is 16-bit signed int but buffer type is not");

            resizeAndClearConvertedSamples(bufferLength);
        } else if constexpr (format == audio_format::AudioFormat::SignedInt24) {
            static_assert(std::same_as<typename decltype(m_convertedSamples)::value_type, ma_uint8>, "Format is 24-bit signed int but buffer is not");

            resizeAndClearConvertedSamples(bufferLength * 3);
        } else if constexpr (format == audio_format::AudioFormat::Float32) {
            static_assert(std::same_as<typename decltype(m_convertedSamples)::value_type, ma_float>, "Format is 32-bit float but buffer is not");

            resizeAndClearConvertedSamples(bufferLength);
        }

        ma_convert_pcm_frames_format(m_convertedSamples.data(), audio_format::toMaFormat(format).value(),
            m_interleavedSamples.data(), ma_format_f32,
            samplesPerChannel, m_encoderConfig.channels, ma_dither_mode_triangle);

        ma_uint64 framesWritten { 0 };

        return ma_encoder_write_pcm_frames(&m_encoder, m_convertedSamples.data(), samplesPerChannel, &framesWritten) == MA_SUCCESS and framesWritten == samplesPerChannel;
    }

private:
    auto resizeAndClearConvertedSamples(const auto size) -> void {
        m_convertedSamples.resize(size);
        std::ranges::fill(m_convertedSamples, static_cast<decltype(m_convertedSamples)::value_type>(0));
    }

    ma_encoder_config m_encoderConfig;
    ma_encoder m_encoder;

    std::vector<float> m_interleavedSamples;
    std::vector<typename SelectMaFormat<format>::type> m_convertedSamples;
};

export template <audio_format::AudioFormat format>
[[nodiscard]] auto makeAudioWriter(std::string_view fileName, const audio_device::SampleRate_t sampleRate, const audio_device::ChannelCount_t channelCount) -> std::expected<std::unique_ptr<AudioWriter>, std::string> {
    if (fileName.empty()) {
        return std::unexpected { "File name can not be empty" };
    }

    if (sampleRate < 44100) {
        return std::unexpected { "Sample rate cannot be less than 44100" };
    }

    if (channelCount == 0 || channelCount > 2) {
        return std::unexpected {"Invalid channel count" };
    }

    try {
        return std::make_unique<AudioWriterWithFormat<format>>(fileName, sampleRate, channelCount);
    } catch (const std::exception& ex) {
        return std::unexpected { ex.what() };
    }
}

}