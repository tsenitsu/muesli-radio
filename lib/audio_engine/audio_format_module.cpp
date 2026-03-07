module;
#include <miniaudio.h>
export module audio_format;

import std;

namespace audio_engine::audio_format {

export enum class AudioFormat {
    Unknown,
    UnsignedInt8,
    SignedInt16,
    SignedInt24,
    SignedInt32,
    Float32
};

export [[nodiscard]] auto constexpr toString(const AudioFormat format) -> std::expected<std::string, std::string> {
    switch (format) {
        case AudioFormat::Unknown:         return "Unknown";
        case AudioFormat::UnsignedInt8:    return "UnsignedInt8";
        case AudioFormat::SignedInt16:     return "SignedInt16";
        case AudioFormat::SignedInt24:     return "SignedInt24";
        case AudioFormat::SignedInt32:     return "SignedInt32";
        case AudioFormat::Float32:         return "Float32";
    };

    return std::unexpected { std::string { "Audio format unknown" } };
}

export [[nodiscard]] auto constexpr toAudioFormat(const ma_format format) -> std::expected<AudioFormat, std::string> {
    switch (format) {
        case ma_format_unknown: return AudioFormat::Unknown;
        case ma_format_u8:      return AudioFormat::UnsignedInt8;
        case ma_format_s16:     return AudioFormat::SignedInt16;
        case ma_format_s24:     return AudioFormat::SignedInt24;
        case ma_format_s32:     return AudioFormat::SignedInt32;
        case ma_format_f32:     return AudioFormat::Float32;
        default: break;
    }

    return std::unexpected { std::string { "ma_format unknown" } };
}

export [[nodiscard]] auto constexpr toMaFormat(const AudioFormat format) -> std::expected<ma_format, std::string> {
    switch (format) {
        case AudioFormat::Unknown:         return ma_format_unknown;
        case AudioFormat::UnsignedInt8:    return ma_format_u8;
        case AudioFormat::SignedInt16:     return ma_format_s16;
        case AudioFormat::SignedInt24:     return ma_format_s24;
        case AudioFormat::SignedInt32:     return ma_format_s32;
        case AudioFormat::Float32:         return ma_format_f32;
    }

    return std::unexpected { std::string { "Audio format unknown" } };
}

}