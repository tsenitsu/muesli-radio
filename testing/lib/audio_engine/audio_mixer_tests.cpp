#include <gtest/gtest.h>

import std;

import channel_routing;
import audio_mixer;
import audio_device;
import audio_buffer;

using namespace audio_engine;

TEST(AudioMixer, makeAudioMixer) {
    auto mixerResult { audio_mixer::makeAudioMixer<float>(0, 0) };
    ASSERT_FALSE(mixerResult.has_value());
    EXPECT_EQ(mixerResult.error(), "Mixer must have at least one input or output channel");

    mixerResult = audio_mixer::makeAudioMixer<float>(1, 0);
    ASSERT_TRUE(mixerResult.has_value());
    EXPECT_NE(mixerResult.value(), nullptr);

    mixerResult = audio_mixer::makeAudioMixer<float>(0, 1);
    ASSERT_TRUE(mixerResult.has_value());
    EXPECT_NE(mixerResult.value(), nullptr);
}

template <class T>
class AudioMixerMock final: public audio_mixer::AudioMixer<T> {
public:
    AudioMixerMock(): audio_mixer::AudioMixer<T> { 5, 3 } {}

    using audio_mixer::AudioMixer<T>::mixerChannelExists;
    using audio_mixer::AudioMixer<T>::m_inputChannels;
    using audio_mixer::AudioMixer<T>::m_outputChannels;
};

class AudioMixerTest: public testing::Test {
protected:
    AudioMixerMock<int> m_audioMixerMock;
};

TEST_F(AudioMixerTest, mixerChannelExists) {
    for (audio_device::ChannelCount_t i { 0 }; i < std::ranges::size(m_audioMixerMock.m_inputChannels); ++i) {
        EXPECT_TRUE(m_audioMixerMock.mixerChannelExists(i, m_audioMixerMock.m_inputChannels));
    }

    for (auto i { static_cast<audio_device::ChannelCount_t>(std::ranges::size(m_audioMixerMock.m_inputChannels)) }; i < std::ranges::size(m_audioMixerMock.m_inputChannels) + 5; ++i) {
        EXPECT_FALSE(m_audioMixerMock.mixerChannelExists(i, m_audioMixerMock.m_inputChannels));
    }

    for (audio_device::ChannelCount_t i { 0 }; i < std::ranges::size(m_audioMixerMock.m_outputChannels); ++i) {
        EXPECT_TRUE(m_audioMixerMock.mixerChannelExists(i, m_audioMixerMock.m_outputChannels));
    }

    for (auto i { static_cast<audio_device::ChannelCount_t>(std::ranges::size(m_audioMixerMock.m_outputChannels)) }; i < std::ranges::size(m_audioMixerMock.m_outputChannels) + 5; ++i) {
        EXPECT_FALSE(m_audioMixerMock.mixerChannelExists(i, m_audioMixerMock.m_outputChannels));
    }
}

TEST_F(AudioMixerTest, inputGain) {
    constexpr int inputGain { 1 };

    for (audio_device::ChannelCount_t i { 0 }; i < std::ranges::size(m_audioMixerMock.m_inputChannels) + 5; ++i) {
        m_audioMixerMock.inputGain(inputGain + static_cast<int>(i), i);
    }

    for (audio_device::ChannelCount_t i { 0 }; i < std::ranges::size(m_audioMixerMock.m_inputChannels); ++i) {
        EXPECT_EQ(m_audioMixerMock.inputGain(i), inputGain + static_cast<int>(i));
    }

    for (auto i { static_cast<audio_device::ChannelCount_t>(std::ranges::size(m_audioMixerMock.m_inputChannels)) }; i < std::ranges::size(m_audioMixerMock.m_inputChannels) + 5; ++i) {
        EXPECT_EQ(m_audioMixerMock.inputGain(i), 0);
    }
}

TEST_F(AudioMixerTest, outputGain) {
    constexpr int outputGain { 1 };

    for (audio_device::ChannelCount_t i { 0 }; i < std::ranges::size(m_audioMixerMock.m_outputChannels) + 5; ++i) {
        m_audioMixerMock.outputGain(outputGain + static_cast<int>(i), i);
    }

    for (audio_device::ChannelCount_t i { 0 }; i < std::ranges::size(m_audioMixerMock.m_outputChannels); ++i) {
        EXPECT_EQ(m_audioMixerMock.outputGain(i), outputGain + static_cast<int>(i));
    }

    for (auto i { static_cast<audio_device::ChannelCount_t>(std::ranges::size(m_audioMixerMock.m_outputChannels)) }; i < std::ranges::size(m_audioMixerMock.m_outputChannels) + 5; ++i) {
        EXPECT_EQ(m_audioMixerMock.outputGain(i), 0);
    }
}

TEST_F(AudioMixerTest, inputRouting) {
    const auto inputRouting  { audio_mixer::ChannelRouting { audio_mixer::Routing_t { 1 }, audio_mixer::Routing_t { 2 } } };

    for (audio_device::ChannelCount_t i { 0 }; i < std::ranges::size(m_audioMixerMock.m_inputChannels) + 5; ++i) {
        m_audioMixerMock.inputRouting(std::make_pair<audio_mixer::ChannelRouting, audio_mixer::ChannelRouting>(
            audio_mixer::ChannelRouting { static_cast<audio_mixer::Routing_t>(inputRouting.m_leftMono.value() + i), static_cast<audio_mixer::Routing_t>(inputRouting.m_right.value() + i) },
            audio_mixer::ChannelRouting { static_cast<audio_mixer::Routing_t>(inputRouting.m_leftMono.value() + i + 1), static_cast<audio_mixer::Routing_t>(inputRouting.m_right.value() + i + 1) }), i);
    }

    for (audio_device::ChannelCount_t i { 0 }; i < std::ranges::size(m_audioMixerMock.m_inputChannels); ++i) {
        EXPECT_EQ(m_audioMixerMock.inputRouting(i), (std::make_pair<audio_mixer::ChannelRouting, audio_mixer::ChannelRouting>(
            audio_mixer::ChannelRouting { static_cast<audio_mixer::Routing_t>(inputRouting.m_leftMono.value() + i), static_cast<audio_mixer::Routing_t>(inputRouting.m_right.value() + i) },
            audio_mixer::ChannelRouting { static_cast<audio_mixer::Routing_t>(inputRouting.m_leftMono.value() + i + 1), static_cast<audio_mixer::Routing_t>(inputRouting.m_right.value() + i + 1) })
        ));
    }

    for (auto i { static_cast<audio_device::ChannelCount_t>(std::ranges::size(m_audioMixerMock.m_inputChannels)) }; i < std::ranges::size(m_audioMixerMock.m_inputChannels) + 5; ++i) {
        EXPECT_EQ(m_audioMixerMock.inputRouting(i), (std::make_pair<audio_mixer::ChannelRouting, audio_mixer::ChannelRouting>(
            audio_mixer::ChannelRouting { },
            audio_mixer::ChannelRouting { })
        ));
    }
}

TEST_F(AudioMixerTest, outputRouting) {
    const auto outputRouting  { audio_mixer::ChannelRouting { audio_mixer::Routing_t { 1 }, audio_mixer::Routing_t { 2 } } };

    for (audio_device::ChannelCount_t i { 0 }; i < std::ranges::size(m_audioMixerMock.m_outputChannels) + 5; ++i) {
        m_audioMixerMock.outputRouting(audio_mixer::ChannelRouting { static_cast<audio_mixer::Routing_t>(outputRouting.m_leftMono.value() + i), static_cast<audio_mixer::Routing_t>(outputRouting.m_right.value() + i) }, i);
    }

    for (audio_device::ChannelCount_t i { 0 }; i < std::ranges::size(m_audioMixerMock.m_outputChannels); ++i) {
        EXPECT_EQ(m_audioMixerMock.outputRouting(i), (audio_mixer::ChannelRouting { static_cast<audio_mixer::Routing_t>(outputRouting.m_leftMono.value() + i), static_cast<audio_mixer::Routing_t>(outputRouting.m_right.value() + i) }));
    }

    for (auto i { static_cast<audio_device::ChannelCount_t>(std::ranges::size(m_audioMixerMock.m_outputChannels)) }; i < std::ranges::size(m_audioMixerMock.m_outputChannels) + 5; ++i) {
        EXPECT_EQ(m_audioMixerMock.outputRouting(i), (audio_mixer::ChannelRouting { }));
    }

    auto monoRouting { audio_mixer::ChannelRouting { audio_mixer::Routing_t { 1 } } };
    m_audioMixerMock.outputRouting(monoRouting, 0);
    EXPECT_EQ(m_audioMixerMock.outputRouting(0), (audio_mixer::ChannelRouting { audio_mixer::Routing_t { 1 }, audio_mixer::Routing_t { 2 } }));
}

TEST_F(AudioMixerTest, processInput) {
    std::array inputAudioSamples { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
    const auto inputAudioBuffer { audio_buffer::makeAudioBuffer<int>(5, 4) };
    inputAudioBuffer->copyFromRawBuffer(inputAudioSamples.data(), 5, 4, false);

    std::array processedInputAudioSamples { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    const auto processedInputAudioBuffer { audio_buffer::makeAudioBuffer<int>(5, 4) };

    m_audioMixerMock.processInput(inputAudioBuffer->view(0), processedInputAudioBuffer->view(0), 0);
    processedInputAudioBuffer->writeToRawBuffer(processedInputAudioSamples.data(), 5, 4, false);
    EXPECT_EQ(processedInputAudioSamples, (std::array { 1, 2, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }));

    m_audioMixerMock.inputGain(2, 1);
    m_audioMixerMock.processInput(inputAudioBuffer->view(1), processedInputAudioBuffer->view(1), 1);
    processedInputAudioBuffer->writeToRawBuffer(processedInputAudioSamples.data(), 5, 4, false);
    EXPECT_EQ(processedInputAudioSamples, (std::array { 1, 2, 3, 4, 10, 12, 14, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }));

    m_audioMixerMock.inputGain(3, 2);
    m_audioMixerMock.processInput(inputAudioBuffer->view(2, 3), processedInputAudioBuffer->view(2, 3), 2);
    processedInputAudioBuffer->writeToRawBuffer(processedInputAudioSamples.data(), 5, 4, false);
    EXPECT_EQ(processedInputAudioSamples, (std::array { 1, 2, 3, 4, 10, 12, 14, 16, 27, 30, 33, 36, 39, 42, 45, 48, 0, 0, 0, 0 }));

    m_audioMixerMock.inputGain(4, 4);
    m_audioMixerMock.processInput(inputAudioBuffer->view(4, 5), processedInputAudioBuffer->view(4, 5), 4);
    processedInputAudioBuffer->writeToRawBuffer(processedInputAudioSamples.data(), 5, 4, false);
    EXPECT_EQ(processedInputAudioSamples, (std::array { 1, 2, 3, 4, 10, 12, 14, 16, 27, 30, 33, 36, 39, 42, 45, 48, 68, 72, 76, 80 }));

    m_audioMixerMock.processInput(inputAudioBuffer->view(6), processedInputAudioBuffer->view(6), 6);
}

TEST_F(AudioMixerTest, processOutput) {
    std::array outputAudioSamples { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
    const auto outputAudioBuffer { audio_buffer::makeAudioBuffer<int>(5, 4) };
    outputAudioBuffer->copyFromRawBuffer(outputAudioSamples.data(), 5, 4, false);

    std::array processedOutputAudioSamples { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    const auto processedOutputAudioBuffer { audio_buffer::makeAudioBuffer<int>(5, 4) };

    m_audioMixerMock.processOutput(outputAudioBuffer->view(0), processedOutputAudioBuffer->view(0), 0);
    processedOutputAudioBuffer->writeToRawBuffer(processedOutputAudioSamples.data(), 5, 4, false);
    EXPECT_EQ(processedOutputAudioSamples, (std::array { 1, 2, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }));

    m_audioMixerMock.outputGain(2, 1);
    m_audioMixerMock.processOutput(outputAudioBuffer->view(1), processedOutputAudioBuffer->view(1), 1);
    processedOutputAudioBuffer->writeToRawBuffer(processedOutputAudioSamples.data(), 5, 4, false);
    EXPECT_EQ(processedOutputAudioSamples, (std::array { 1, 2, 3, 4, 10, 12, 14, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }));

    m_audioMixerMock.outputGain(3, 2);
    m_audioMixerMock.processOutput(outputAudioBuffer->view(2, 3), processedOutputAudioBuffer->view(2, 3), 2);
    processedOutputAudioBuffer->writeToRawBuffer(processedOutputAudioSamples.data(), 5, 4, false);
    EXPECT_EQ(processedOutputAudioSamples, (std::array { 1, 2, 3, 4, 10, 12, 14, 16, 27, 30, 33, 36, 39, 42, 45, 48, 0, 0, 0, 0 }));

    m_audioMixerMock.outputGain(4, 4);
    m_audioMixerMock.processOutput(outputAudioBuffer->view(4, 5), processedOutputAudioBuffer->view(4, 5), 4);
    processedOutputAudioBuffer->writeToRawBuffer(processedOutputAudioSamples.data(), 5, 4, false);
    EXPECT_EQ(processedOutputAudioSamples, (std::array { 1, 2, 3, 4, 10, 12, 14, 16, 27, 30, 33, 36, 39, 42, 45, 48, 0, 0, 0, 0 }));

    m_audioMixerMock.processOutput(outputAudioBuffer->view(6), processedOutputAudioBuffer->view(6), 6);
}

TEST_F(AudioMixerTest, mix) {
    std::array inputSamples { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    const auto inputBuffer { audio_buffer::makeAudioBuffer<int>(2, 5) };
    inputBuffer->copyFromRawBuffer(inputSamples.data(), 2, 5, false);

    std::array outputSamples { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    const auto outputBuffer { audio_buffer::makeAudioBuffer<int>(2, 5) };
    outputBuffer->copyFromRawBuffer(outputSamples.data(), 2, 5, false);

    // Both mono
    m_audioMixerMock.mix(inputBuffer->view(0), outputBuffer->view(0));
    outputBuffer->writeToRawBuffer(outputSamples.data(), 2, 5, false);

    EXPECT_EQ(outputSamples, (std::array { 1, 2, 3, 4, 5, 0, 0, 0, 0, 0 }));
    outputBuffer->clear();

    // Input mono, output stereo
    m_audioMixerMock.mix(inputBuffer->view(0), outputBuffer->view(0, 1));
    outputBuffer->writeToRawBuffer(outputSamples.data(), 2, 5, false);

    EXPECT_EQ(outputSamples, (std::array { 1, 2, 3, 4, 5, 1, 2, 3, 4, 5 }));
    outputBuffer->clear();

    // Input stereo, output mono
    m_audioMixerMock.mix(inputBuffer->view(0, 1), outputBuffer->view(0));
    outputBuffer->writeToRawBuffer(outputSamples.data(), 2, 5, false);

    EXPECT_EQ(outputSamples, (std::array { 3, 4, 5, 6, 7, 0, 0, 0, 0, 0 }));
}