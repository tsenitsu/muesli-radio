#include <gtest/gtest.h>

import std;
import audio_buffer;

using namespace audio_engine;

TEST(AudioBuffer, makeAudioBuffer) {
    const auto buffer { audio_buffer::makeAudioBuffer<int>(2, 5) };

    EXPECT_EQ(buffer->numberOfChannels(), 2);
    EXPECT_EQ(buffer->bufferLength(), 5);

    const auto anotherBuffer { audio_buffer::makeAudioBuffer<int>(3, 1) };

    EXPECT_EQ(anotherBuffer->numberOfChannels(), 3);
    EXPECT_EQ(anotherBuffer->bufferLength(), 1);
}

TEST(AudioBuffer, clearAudioBuffer) {
    std::array samples { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    const auto buffer { audio_buffer::makeAudioBuffer<int>(3, 3) };
    buffer->copyFromRawBuffer(samples.data(), 3, 3, false);

    buffer->clear(3);
    buffer->writeToRawBuffer(samples.data(), 3, 3, false);
    EXPECT_EQ(samples, (std::array { 1, 2, 3, 4, 5, 6, 7, 8, 9 }));

    buffer->clear(1);
    buffer->writeToRawBuffer(samples.data(), 3, 3, false);
    EXPECT_EQ(samples, (std::array { 1, 2, 3, 0, 0, 0, 7, 8, 9 }));

    buffer->clear(0);
    buffer->writeToRawBuffer(samples.data(), 3, 3, false);
    EXPECT_EQ(samples, (std::array { 0, 0, 0, 0, 0, 0, 7, 8, 9 }));

    buffer->clear(2);
    buffer->writeToRawBuffer(samples.data(), 3, 3, false);
    EXPECT_EQ(samples, (std::array { 0, 0, 0, 0, 0, 0, 0, 0, 0 }));

    samples = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    buffer->clear();
    buffer->writeToRawBuffer(samples.data(), 3, 3, false);
    EXPECT_EQ(samples, (std::array { 0, 0, 0, 0, 0, 0, 0, 0, 0 }));
}

TEST(AudioBuffer, addFrom) {
    std::array samplesDest { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    std::array samplesSrc { 11, 22, 33, 44 };

    const auto bufferDest { audio_buffer::makeAudioBuffer<int>(3, 3) };
    bufferDest->copyFromRawBuffer(samplesDest.data(), 3, 3, false);

    const auto bufferSrc { audio_buffer::makeAudioBuffer<int>(2, 2) };
    bufferSrc->copyFromRawBuffer(samplesSrc.data(), 2, 2, false);

    bufferDest->addFrom(*bufferSrc, 2, 0);
    bufferDest->writeToRawBuffer(samplesDest.data(), 3, 3, false);
    EXPECT_EQ(samplesDest, (std::array { 1, 2, 3, 4, 5, 6, 7, 8, 9 }));

    bufferDest->addFrom(*bufferSrc, 0, 3);
    bufferDest->writeToRawBuffer(samplesDest.data(), 3, 3, false);
    EXPECT_EQ(samplesDest, (std::array { 1, 2, 3, 4, 5, 6, 7, 8, 9 }));

    bufferDest->addFrom(*bufferSrc, 1, 2);
    bufferDest->writeToRawBuffer(samplesDest.data(), 3, 3, false);
    EXPECT_EQ(samplesDest, (std::array { 1, 2, 3, 4, 5, 6, 40, 52, 9 }));

    bufferDest->addFrom(*bufferSrc, 0, 0);
    bufferDest->writeToRawBuffer(samplesDest.data(), 3, 3, false);
    EXPECT_EQ(samplesDest, (std::array { 12, 24, 3, 4, 5, 6, 40, 52, 9 }));

    bufferSrc->addFrom(*bufferDest, 1, 1);
    bufferSrc->writeToRawBuffer(samplesSrc.data(), 2, 2, false);
    EXPECT_EQ(samplesSrc, (std::array { 11, 22, 37, 49 }));
}

TEST(AudioBuffer, copyFrom) {
    std::array samplesDest { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    std::array samplesSrc { 11, 22, 33, 44 };

    const auto bufferDest { audio_buffer::makeAudioBuffer<int>(3, 3) };
    bufferDest->copyFromRawBuffer(samplesDest.data(), 3, 3, false);

    const auto bufferSrc { audio_buffer::makeAudioBuffer<int>(2, 2) };
    bufferSrc->copyFromRawBuffer(samplesSrc.data(), 2, 2, false);

    bufferDest->copyFrom(*bufferSrc, 2, 0);
    bufferDest->writeToRawBuffer(samplesDest.data(), 3, 3, false);
    EXPECT_EQ(samplesDest, (std::array { 1, 2, 3, 4, 5, 6, 7, 8, 9 }));

    bufferDest->copyFrom(*bufferSrc, 0, 3);
    bufferDest->writeToRawBuffer(samplesDest.data(), 3, 3, false);
    EXPECT_EQ(samplesDest, (std::array { 1, 2, 3, 4, 5, 6, 7, 8, 9 }));

    bufferDest->copyFrom(*bufferSrc, 1, 2);
    bufferDest->writeToRawBuffer(samplesDest.data(), 3, 3, false);
    EXPECT_EQ(samplesDest, (std::array { 1, 2, 3, 4, 5, 6, 33, 44, 9 }));

    bufferDest->copyFrom(*bufferSrc, 0, 0);
    bufferDest->writeToRawBuffer(samplesDest.data(), 3, 3, false);
    EXPECT_EQ(samplesDest, (std::array { 11, 22, 3, 4, 5, 6, 33, 44, 9 }));

    bufferSrc->copyFrom(*bufferDest, 1, 1);
    bufferSrc->writeToRawBuffer(samplesSrc.data(), 2, 2, false);
    EXPECT_EQ(samplesSrc, (std::array { 11, 22, 4, 5 }));
}

TEST(AudioBuffer, assignmentOperator) {
    std::array samplesDest { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    std::array samplesSrc { 11, 22, 33, 44 };

    const auto bufferDest { audio_buffer::makeAudioBuffer<int>(3, 3) };
    bufferDest->copyFromRawBuffer(samplesDest.data(), 3, 3, false);

    const auto bufferSrc { audio_buffer::makeAudioBuffer<int>(2, 2) };
    bufferSrc->copyFromRawBuffer(samplesSrc.data(), 2, 2, false);

    *bufferDest = *bufferSrc;
    bufferDest->writeToRawBuffer(samplesDest.data(), 3, 3, false);
    EXPECT_EQ(samplesDest, (std::array { 11, 22, 3, 33, 44, 6, 7, 8, 9 }));

    std::array samplesDest2 { 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200 };

    const auto bufferDest2 { audio_buffer::makeAudioBuffer<int>(4, 3) };
    bufferDest2->copyFromRawBuffer(samplesDest2.data(), 4, 3, false);

    *bufferSrc = *bufferDest2;

    bufferSrc->writeToRawBuffer(samplesSrc.data(), 2, 2, false);
    EXPECT_EQ(samplesSrc, (std::array { 100, 200, 400, 500 }));

    *bufferDest2 = *bufferDest;

    bufferDest2->writeToRawBuffer(samplesDest2.data(), 4, 3, false);
    EXPECT_EQ(samplesDest2, (std::array { 11, 22, 3, 33, 44, 6, 7, 8, 9, 1000, 1100, 1200 }));
}

TEST(AudioBuffer, additionAssignmentOperator) {
    std::array samplesDest { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    std::array samplesSrc { 11, 22, 33, 44 };

    const auto bufferDest { audio_buffer::makeAudioBuffer<int>(3, 3) };
    bufferDest->copyFromRawBuffer(samplesDest.data(), 3, 3, false);

    const auto bufferSrc { audio_buffer::makeAudioBuffer<int>(2, 2) };
    bufferSrc->copyFromRawBuffer(samplesSrc.data(), 2, 2, false);

    *bufferDest += *bufferDest;

    bufferDest->writeToRawBuffer(samplesDest.data(), 3, 3, false);
    EXPECT_EQ(samplesDest, (std::array { 1, 2, 3, 4, 5, 6, 7, 8, 9 }));

    *bufferDest += *bufferSrc;

    bufferDest->writeToRawBuffer(samplesDest.data(), 3, 3, false);
    EXPECT_EQ(samplesDest, (std::array { 12, 24, 3, 37, 49, 6, 7, 8, 9 }));

    std::array samplesDest2 { 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200 };

    const auto bufferDest2 { audio_buffer::makeAudioBuffer<int>(4, 3) };
    bufferDest2->copyFromRawBuffer(samplesDest2.data(), 4, 3, false);

    *bufferSrc += *bufferDest2;

    bufferSrc->writeToRawBuffer(samplesSrc.data(), 2, 2, false);
    EXPECT_EQ(samplesSrc, (std::array { 111, 222, 433, 544 }));

    *bufferDest2 += *bufferDest;

    bufferDest2->writeToRawBuffer(samplesDest2.data(), 4, 3, false);
    EXPECT_EQ(samplesDest2, (std::array { 112, 224, 303, 437, 549, 606, 707, 808, 909, 1000, 1100, 1200 }));
}

TEST(AudioBuffer, deinterleave) {
    std::array audioSamples { 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6 };
    const auto audioBuffer { audio_buffer::makeAudioBuffer<int>(6, 4) };
    audioBuffer->copyFromRawBuffer(audioSamples.data(), 6, 4, true);

    audioBuffer->writeToRawBuffer(audioSamples.data(), 6, 4, false);
    EXPECT_EQ(audioSamples, (std::array { 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6 }));

    std::array audioSamples2 { 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3 };
    const auto audioBuffer2 { audio_buffer::makeAudioBuffer<int>(3, 5) };
    audioBuffer2->copyFromRawBuffer(audioSamples2.data(), 3, 5, true);

    audioBuffer2->writeToRawBuffer(audioSamples2.data(), 3, 5, false);
    EXPECT_EQ(audioSamples2, (std::array { 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3 }));
}

TEST(AudioBuffer, interleave) {
    std::array audioSamples { 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6 };
    const auto audioBuffer { audio_buffer::makeAudioBuffer<int>(6, 4) };
    audioBuffer->copyFromRawBuffer(audioSamples.data(), 6, 4, false);

    audioBuffer->writeToRawBuffer(audioSamples.data(), 6, 4, true);
    EXPECT_EQ(audioSamples, (std::array { 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6 }));

    std::array audioSamples2 { 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3 };
    const auto audioBuffer2 { audio_buffer::makeAudioBuffer<int>(3, 5) };
    audioBuffer2->copyFromRawBuffer(audioSamples2.data(), 3, 5, false);

    audioBuffer2->writeToRawBuffer(audioSamples2.data(), 3, 5, true);
    EXPECT_EQ(audioSamples2, (std::array { 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3 }));
}