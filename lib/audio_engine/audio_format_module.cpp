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

export [[nodiscard]] auto toString(AudioFormat format) -> std::expected<std::string, std::string>;
export [[nodiscard]] auto toAudioFormat(ma_format format) -> std::expected<AudioFormat, std::string>;
export [[nodiscard]] auto toMaFormat(AudioFormat format) -> std::expected<ma_format, std::string>;

}