#include <gtest/gtest.h>

import std;
import ring_audio_buffer;
import audio_device;
import audio_stream_params;
import audio_buffer;

using namespace audio_engine;

TEST(RingAudioBuffer, makeRingAudioBuffer) {
    auto rbResult { ring_audio_buffer::makeRingAudioBuffer<float>(1, 1) };
    EXPECT_TRUE(rbResult.has_value());

    rbResult = ring_audio_buffer::makeRingAudioBuffer<float>(0, 0);
    ASSERT_FALSE(rbResult.has_value());
    ASSERT_EQ(rbResult.error(), std::string { "Unable to initialize ring buffer" });
}

TEST(RingAudioBuffer, enqueueDequeue) {
    std::array inputSamples { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    const auto inputAudioBuffer { audio_buffer::makeAudioBuffer<int>(3, 3)};
    inputAudioBuffer->copyFromRawBuffer(inputSamples.data(), 3, 3);

    ring_audio_buffer::RingAudioBuffer<int> rb {audio_device::ChannelCount_t { 3 }, audio_stream_params::BufferLength_t { 9 } };

    for (int i { 0 }; i < 3; ++i) {
        EXPECT_TRUE(rb.enqueue(*inputAudioBuffer));
        std::ranges::transform(inputSamples, inputSamples.begin(), [] (auto sample) { return ++sample; });
        inputAudioBuffer->copyFromRawBuffer(inputSamples.data(), 3, 3);
    }

    EXPECT_FALSE(rb.enqueue(*inputAudioBuffer));

    constexpr std::array expectedResult { 0, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8, 9, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

    std::array<int, 27> outputSamples {};
    const auto outputAudioBuffer { audio_buffer::makeAudioBuffer<int>(3, 3)};
    outputAudioBuffer->copyFromRawBuffer(outputSamples.data(), 3, 3);

    EXPECT_TRUE(rb.dequeue(*outputAudioBuffer));
    outputAudioBuffer->writeToRawBuffer(outputSamples.data(), 3, 9);

    EXPECT_EQ(outputSamples, expectedResult);
    EXPECT_TRUE(rb.dequeue(*outputAudioBuffer));
}

TEST(RingAudioBuffer, wraparound) {
    // 2 channels, capacity 8 frames
    ring_audio_buffer::RingAudioBuffer<int> rb {
        audio_device::ChannelCount_t { 2 },
        audio_stream_params::BufferLength_t { 8 }
    };

    // Advance write/read pointers to 6 then discard
    constexpr std::array firstInput { 1,2, 3,4, 5,6, 7,8, 9,10, 11,12 };
    const auto firstBuffer { audio_buffer::makeAudioBuffer<int>(2, 6) };
    firstBuffer->copyFromRawBuffer(firstInput.data(), 2, 6, true);
    ASSERT_TRUE(rb.enqueue(*firstBuffer));

    const auto discardBuffer { audio_buffer::makeAudioBuffer<int>(0, 0) };
    ASSERT_TRUE(rb.dequeue(*discardBuffer));
    // write ptr = 6, read ptr = 6

    // Enqueue 2 frames — fits in the 2 contiguous slots remaining (pos 6,7)
    // write ptr wraps to 0
    const std::array secondInput { 100,200, 300,400 };
    const auto secondBuffer { audio_buffer::makeAudioBuffer<int>(2, 2) };
    secondBuffer->copyFromRawBuffer(secondInput.data(), 2, 2, true);
    ASSERT_TRUE(rb.enqueue(*secondBuffer));

    // Enqueue 4 frames — fits in the 4 contiguous slots at start (pos 0-3)
    // write ptr advances to 4
    constexpr std::array thirdInput { 500,600, 700,800, 900,1000, 1100,1200 };
    const auto thirdBuffer { audio_buffer::makeAudioBuffer<int>(2, 4) };
    thirdBuffer->copyFromRawBuffer(thirdInput.data(), 2, 4, true);
    ASSERT_TRUE(rb.enqueue(*thirdBuffer));
    // read ptr = 6, write ptr = 4: dequeue must read two chunks

    // Dequeue: chunk1 (offset=0) = 2 frames from pos 6,7
    //          chunk2 (offset=2) = 4 frames from pos 0-3
    const auto outputBuffer { audio_buffer::makeAudioBuffer<int>(0, 0) };
    ASSERT_TRUE(rb.dequeue(*outputBuffer));

    ASSERT_EQ(outputBuffer->numberOfChannels(), 2);
    ASSERT_EQ(outputBuffer->bufferLength(), 6);

    std::array<int, 12> outputSamples {};
    outputBuffer->writeToRawBuffer(outputSamples.data(), 2, 6, true);

    // Expected: secondInput followed by thirdInput, interleaved
    constexpr std::array expectedResult { 100,200, 300,400, 500,600, 700,800, 900,1000, 1100,1200 };
    EXPECT_EQ(outputSamples, expectedResult);
}

TEST(RingAudioBuffer, enqueue_wraparound) {
    // 2 channels, capacity 8 frames
    ring_audio_buffer::RingAudioBuffer<int> rb {
        audio_device::ChannelCount_t { 2 },
        audio_stream_params::BufferLength_t { 8 }
    };

    // Advance write/read pointers to 6, then discard
    constexpr std::array firstInput { 1,2, 3,4, 5,6, 7,8, 9,10, 11,12 };
    const auto firstBuffer { audio_buffer::makeAudioBuffer<int>(2, 6) };
    firstBuffer->copyFromRawBuffer(firstInput.data(), 2, 6, true);
    ASSERT_TRUE(rb.enqueue(*firstBuffer));

    const auto discardBuffer { audio_buffer::makeAudioBuffer<int>(0, 0) };
    ASSERT_TRUE(rb.dequeue(*discardBuffer));
    // write ptr = 6, read ptr = 6

    // Single enqueue of 4 frames spanning the boundary:
    // chunk1 (frames 0-1 of input) → ring positions 6-7
    // chunk2 (frames 2-3 of input) → ring positions 0-1  ← exercises enqueue wraparound
    constexpr std::array wrapInput { 100,200, 300,400, 500,600, 700,800 };
    const auto wrapBuffer { audio_buffer::makeAudioBuffer<int>(2, 4) };
    wrapBuffer->copyFromRawBuffer(wrapInput.data(), 2, 4, true);
    ASSERT_TRUE(rb.enqueue(*wrapBuffer));
    // write ptr = 2, read ptr = 6

    // Dequeue and verify round-trip integrity
    const auto outputBuffer { audio_buffer::makeAudioBuffer<int>(0, 0) };
    ASSERT_TRUE(rb.dequeue(*outputBuffer));

    ASSERT_EQ(outputBuffer->numberOfChannels(), 2);
    ASSERT_EQ(outputBuffer->bufferLength(), 4);

    std::array<int, 8> outputSamples {};
    outputBuffer->writeToRawBuffer(outputSamples.data(), 2, 4, true);

    EXPECT_EQ(outputSamples, wrapInput);
}

TEST(RingAudioBuffer, reset) {
    // 2 channels, capacity 8 frames
    ring_audio_buffer::RingAudioBuffer<int> rb {
        audio_device::ChannelCount_t { 2 },
        audio_stream_params::BufferLength_t { 8 }
    };

    // Enqueue some frames and leave them unread
    constexpr std::array input { 1,2, 3,4, 5,6, 7,8 };
    const auto inputBuffer { audio_buffer::makeAudioBuffer<int>(2, 4) };
    inputBuffer->copyFromRawBuffer(input.data(), 2, 4, true);
    ASSERT_TRUE(rb.enqueue(*inputBuffer));

    // Reset before draining — simulates stopping a recording with unread backlog still in the buffer
    rb.reset();

    // After reset, there should be nothing to dequeue
    const auto emptyCheckBuffer { audio_buffer::makeAudioBuffer<int>(0, 0) };
    ASSERT_TRUE(rb.dequeue(*emptyCheckBuffer));
    EXPECT_EQ(emptyCheckBuffer->numberOfChannels(), 2);
    EXPECT_EQ(emptyCheckBuffer->bufferLength(), 0);

    // Ring buffer must still be fully usable after reset: fill it completely again
    constexpr std::array secondInput { 10,20, 30,40, 50,60, 70,80, 90,100, 110,120, 130,140, 150,160 };
    const auto secondBuffer { audio_buffer::makeAudioBuffer<int>(2, 8) };
    secondBuffer->copyFromRawBuffer(secondInput.data(), 2, 8, true);
    ASSERT_TRUE(rb.enqueue(*secondBuffer));

    // Buffer is now full (8/8 frames), a further enqueue must fail
    const auto overflowBuffer { audio_buffer::makeAudioBuffer<int>(2, 1) };
    EXPECT_FALSE(rb.enqueue(*overflowBuffer));

    // Dequeue should return exactly what was written post-reset, from the start
    const auto outputBuffer { audio_buffer::makeAudioBuffer<int>(0, 0) };
    ASSERT_TRUE(rb.dequeue(*outputBuffer));

    ASSERT_EQ(outputBuffer->numberOfChannels(), 2);
    ASSERT_EQ(outputBuffer->bufferLength(), 8);

    std::array<int, 16> outputSamples {};
    outputBuffer->writeToRawBuffer(outputSamples.data(), 2, 8, true);

    EXPECT_EQ(outputSamples, secondInput);
}

TEST(RingAudioBuffer, resetAfterWraparound) {
    // 2 channels, capacity 8 frames
    ring_audio_buffer::RingAudioBuffer<int> rb {
        audio_device::ChannelCount_t { 2 },
        audio_stream_params::BufferLength_t { 8 }
    };

    // Advance write/read pointers past the wraparound boundary, like the existing wraparound test
    constexpr std::array firstInput { 1,2, 3,4, 5,6, 7,8, 9,10, 11,12 };
    const auto firstBuffer { audio_buffer::makeAudioBuffer<int>(2, 6) };
    firstBuffer->copyFromRawBuffer(firstInput.data(), 2, 6, true);
    ASSERT_TRUE(rb.enqueue(*firstBuffer));

    const auto discardBuffer { audio_buffer::makeAudioBuffer<int>(0, 0) };
    ASSERT_TRUE(rb.dequeue(*discardBuffer));
    // write ptr = 6, read ptr = 6

    // Enqueue 4 more frames, unread — leaves a backlog straddling the wrap boundary
    constexpr std::array secondInput { 100,200, 300,400, 500,600, 700,800 };
    const auto secondBuffer { audio_buffer::makeAudioBuffer<int>(2, 4) };
    secondBuffer->copyFromRawBuffer(secondInput.data(), 2, 4, true);
    ASSERT_TRUE(rb.enqueue(*secondBuffer));
    // write ptr = 2, read ptr = 6 — 4 unread frames left over, wrapped

    // Reset must clear this backlog regardless of wraparound state
    rb.reset();

    const auto emptyCheckBuffer { audio_buffer::makeAudioBuffer<int>(0, 0) };
    ASSERT_TRUE(rb.dequeue(*emptyCheckBuffer));
    EXPECT_EQ(emptyCheckBuffer->bufferLength(), 0);

    // Buffer must behave as freshly constructed: full capacity available again
    constexpr std::array thirdInput { 1,1, 2,2, 3,3, 4,4, 5,5, 6,6, 7,7, 8,8 };
    const auto thirdBuffer { audio_buffer::makeAudioBuffer<int>(2, 8) };
    thirdBuffer->copyFromRawBuffer(thirdInput.data(), 2, 8, true);
    ASSERT_TRUE(rb.enqueue(*thirdBuffer));

    const auto outputBuffer { audio_buffer::makeAudioBuffer<int>(0, 0) };
    ASSERT_TRUE(rb.dequeue(*outputBuffer));
    ASSERT_EQ(outputBuffer->bufferLength(), 8);

    std::array<int, 16> outputSamples {};
    outputBuffer->writeToRawBuffer(outputSamples.data(), 2, 8, true);
    EXPECT_EQ(outputSamples, thirdInput);
}