#include <gtest/gtest.h>

import std;

import audio_recorder;
import channel_routing;
import audio_format;

using namespace audio_engine;

TEST(AudioRecorder, makeAudioRecorder) {
    auto audioRecorderResult { audio_recorder::makeAudioRecorder(44100, audio_format::AudioFormat::SignedInt16,
        {}, { audio_mixer::ChannelRouting {audio_mixer::Routing_t { 0 }, audio_mixer::Routing_t { 1 } } }) };

    ASSERT_FALSE(audioRecorderResult.has_value());
    EXPECT_EQ(audioRecorderResult.error(), "No files provided");

    audioRecorderResult = audio_recorder::makeAudioRecorder(44100, audio_format::AudioFormat::SignedInt16,
                                                            { "test", "test" }, { audio_mixer::ChannelRouting {audio_mixer::Routing_t { 0 }, audio_mixer::Routing_t { 1 } },
                                                            audio_mixer::ChannelRouting {audio_mixer::Routing_t { 0 }, audio_mixer::Routing_t { 1 } }});

    ASSERT_FALSE(audioRecorderResult.has_value());
    EXPECT_EQ(audioRecorderResult.error(), "Duplicate file names provided");

    audioRecorderResult = audio_recorder::makeAudioRecorder(44100, audio_format::AudioFormat::SignedInt16,
                                                            { "test" }, {});

    ASSERT_FALSE(audioRecorderResult.has_value());
    EXPECT_EQ(audioRecorderResult.error(), "No routing list provided");

    audioRecorderResult = audio_recorder::makeAudioRecorder(44100, audio_format::AudioFormat::SignedInt16,
                                                            { "test1", "test2" }, { audio_mixer::ChannelRouting {audio_mixer::Routing_t { 0 }, audio_mixer::Routing_t { 1 } } });

    ASSERT_FALSE(audioRecorderResult.has_value());
    EXPECT_EQ(audioRecorderResult.error(), "Mismatch between number of files and number of channel routing");

    audioRecorderResult = audio_recorder::makeAudioRecorder(44100, audio_format::AudioFormat::SignedInt32,
                                                        { "test" }, { audio_mixer::ChannelRouting {audio_mixer::Routing_t { 0 }, audio_mixer::Routing_t { 1 } } });

    ASSERT_FALSE(audioRecorderResult.has_value());
    EXPECT_EQ(audioRecorderResult.error(), "Unsupported audio format");

    audioRecorderResult =  audio_recorder::makeAudioRecorder(44100, audio_format::AudioFormat::SignedInt16,
                                                             { "test" }, { audio_mixer::ChannelRouting {} });

    ASSERT_FALSE(audioRecorderResult.has_value());
    EXPECT_EQ(audioRecorderResult.error(), "No audio writers provided");

    audioRecorderResult =  audio_recorder::makeAudioRecorder(44100, audio_format::AudioFormat::SignedInt16,
                                                             { "test1", "test2" }, { audio_mixer::ChannelRouting {audio_mixer::Routing_t { 0 }, audio_mixer::Routing_t { 1 } }, audio_mixer::ChannelRouting {} });

    ASSERT_TRUE(audioRecorderResult.has_value());
}