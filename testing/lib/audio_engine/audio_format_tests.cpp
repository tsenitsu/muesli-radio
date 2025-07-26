#include <gtest/gtest.h>
#include <miniaudio.h>

import std;
import audio_format;

using namespace audio_engine;

const std::map<audio_format::AudioFormat, std::string_view> audioFormatToString {
    { audio_format::AudioFormat::Unknown,       "Unknown" },
    { audio_format::AudioFormat::UnsignedInt8,  "UnsignedInt8" },
    { audio_format::AudioFormat::SignedInt16,   "SignedInt16" },
    { audio_format::AudioFormat::SignedInt24,   "SignedInt24" },
    { audio_format::AudioFormat::SignedInt32,   "SignedInt32" },
    { audio_format::AudioFormat::Float32,       "Float32" }
};

const std::map<ma_format, audio_format::AudioFormat> maFormatToAudioFormat = {
    { ma_format_unknown,    audio_format::AudioFormat::Unknown },
    { ma_format_u8,         audio_format::AudioFormat::UnsignedInt8 },
    { ma_format_s16,        audio_format::AudioFormat::SignedInt16 },
    { ma_format_s24,        audio_format::AudioFormat::SignedInt24 },
    { ma_format_s32,        audio_format::AudioFormat::SignedInt32 },
    { ma_format_f32,        audio_format::AudioFormat::Float32 }
};

TEST(audio_format, toString) {
    for (auto const& [format, formatString]: audioFormatToString) {
        EXPECT_EQ(audio_format::toString(format).value(), formatString);
    }

    EXPECT_EQ(audio_format::toString(static_cast<audio_format::AudioFormat>(55)), std::unexpected { "Audio format unknown" });
}

TEST(audio_format, toAudioFormatAndMaFormat) {
    for (auto const& [maFormat, audioFormat]: maFormatToAudioFormat) {
        EXPECT_EQ(audio_format::toAudioFormat(maFormat).value(), audioFormat);
        EXPECT_EQ(audio_format::toMaFormat(audioFormat).value(), maFormat);
    }

    EXPECT_EQ(audio_format::toAudioFormat(ma_format_count), std::unexpected { "ma_format unknown" });
}