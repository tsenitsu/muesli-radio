#include <gtest/gtest.h>
#include <miniaudio.h>

import std;
import audio_device;
import audio_writer;
import audio_format;
import audio_buffer;

using namespace audio_engine;

TEST(AudioWriter, makeAudioWriter) {
    std::string_view fileName { "test" };
    std::filesystem::remove((std::string { fileName }).append(".wav"));

    auto audioWriterResult { audio_recorder::makeAudioWriter<audio_format::AudioFormat::Float32>("", audio_device::SampleRate_t { 44100 }, audio_device::ChannelCount_t { 2 }) };

    ASSERT_FALSE(audioWriterResult.has_value());
    EXPECT_EQ(audioWriterResult.error(), "File name can not be empty");

    audioWriterResult = audio_recorder::makeAudioWriter<audio_format::AudioFormat::Float32>(fileName, audio_device::SampleRate_t { 40000 }, audio_device::ChannelCount_t { 2 }) ;

    ASSERT_FALSE(audioWriterResult.has_value());
    EXPECT_EQ(audioWriterResult.error(), "Sample rate cannot be less than 44100");

    audioWriterResult = audio_recorder::makeAudioWriter<audio_format::AudioFormat::Float32>(fileName, audio_device::SampleRate_t { 44100 }, audio_device::ChannelCount_t { 0 }) ;

    ASSERT_FALSE(audioWriterResult.has_value());
    EXPECT_EQ(audioWriterResult.error(), "Invalid channel count");

    audioWriterResult = audio_recorder::makeAudioWriter<audio_format::AudioFormat::Float32>(fileName, audio_device::SampleRate_t { 44100 }, audio_device::ChannelCount_t { 3 }) ;

    ASSERT_FALSE(audioWriterResult.has_value());
    EXPECT_EQ(audioWriterResult.error(), "Invalid channel count");

    audioWriterResult = audio_recorder::makeAudioWriter<audio_format::AudioFormat::Float32>(fileName, audio_device::SampleRate_t { 44100 }, audio_device::ChannelCount_t { 2 }) ;
    ASSERT_TRUE(audioWriterResult.has_value());

    // Release the file otherwise remove will throw exception
    audioWriterResult.value().reset();

    EXPECT_TRUE(std::filesystem::remove((std::string { fileName }).append(".wav")));
}

auto decodeWav(std::string_view fileName, ma_format format, unsigned int channels, unsigned int sampleRate, void *pFrames, ma_uint64 framesToRead) -> void {
    ma_decoder decoder;
    ma_decoder_config config = ma_decoder_config_init(format, channels, sampleRate);
    config.encodingFormat = ma_encoding_format_wav;

    if (ma_result result = ma_decoder_init_file(fileName.data(), &config, &decoder); result != MA_SUCCESS) {
        ma_decoder_uninit(&decoder);
        throw std::runtime_error("Failed to initialize decoder");
    }

    ma_uint64 framesRead { 0 };
    ma_result result = ma_decoder_read_pcm_frames(&decoder, pFrames, framesToRead, &framesRead);
    if (result != MA_SUCCESS || framesRead < framesToRead) {
        ma_decoder_uninit(&decoder);
        throw std::runtime_error("Failed to read from WAV");
    }

    ma_decoder_uninit(&decoder);
}

TEST(AudioWriter, write) {
    std::string_view fileName { "writeTest" };

    constexpr auto formats { std::array { ma_format_s16, ma_format_s24, ma_format_f32 } };

    for (unsigned int j { 1 }; j < 3; ++j) {
        constexpr auto numberOfSamples { 12 };
        const auto numberOfChannels { audio_device::ChannelCount_t { j } };
        const auto numberOfFrames { numberOfSamples / numberOfChannels };

        for (unsigned int i { 0 }; i < 3; ++i) {
            std::filesystem::remove((std::string { fileName }).append(".wav"));

            const auto format { formats[i] };
            std::expected<std::unique_ptr<audio_recorder::AudioWriter>, std::string> audioWriterResult {};

            if (format == ma_format_s16)
                audioWriterResult = audio_recorder::makeAudioWriter<audio_format::AudioFormat::SignedInt16>(fileName, audio_device::SampleRate_t { 44100 }, numberOfChannels);
            else if (format == ma_format_s24)
                audioWriterResult = audio_recorder::makeAudioWriter<audio_format::AudioFormat::SignedInt24>(fileName, audio_device::SampleRate_t { 44100 }, numberOfChannels);
            else if (format == ma_format_f32)
                audioWriterResult = audio_recorder::makeAudioWriter<audio_format::AudioFormat::Float32>(fileName, audio_device::SampleRate_t { 44100 }, numberOfChannels);
            else
                throw std::runtime_error { "Unsupported format" };

            ASSERT_TRUE(audioWriterResult.has_value());

            auto audioWriter { std::move(audioWriterResult.value()) };

            auto audioSamples { std::array<float, numberOfSamples> { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.0f } };
            auto audioBuffer { audio_buffer::makeAudioBuffer<float>(numberOfChannels, numberOfFrames) };
            audioBuffer->copyFromRawBuffer(audioSamples.data(), numberOfChannels, numberOfFrames, false);

            if (numberOfChannels == 1)
                EXPECT_TRUE(audioWriter->write(audioBuffer->view(0)));
            else if (numberOfChannels == 2)
                EXPECT_TRUE(audioWriter->write(audioBuffer->view(0, 1)));
            else
                throw std::runtime_error { "Unsupported number of channels" };

            // Need to release the file to read it
            audioWriter.reset(nullptr);

            std::array<float, numberOfSamples> result {};

            decodeWav(std::string { fileName }.append(".wav"), ma_format_f32, numberOfChannels, 44100, result.data(), numberOfFrames);

            if (numberOfChannels == 2)
                // Need to interleave samples, for simplicity we interleave original samples instead of decoded ones
                audioBuffer->writeToRawBuffer(audioSamples.data(), numberOfChannels, numberOfFrames, true);

            for (const auto [readSample, ogSample]: std::ranges::views::zip(result, audioSamples)) {
                constexpr float tolerance = 1e-4f;
                EXPECT_NEAR(readSample, ogSample, tolerance);
            }

            EXPECT_TRUE(std::filesystem::remove((std::string { fileName }).append(".wav")));
        }
    }
}