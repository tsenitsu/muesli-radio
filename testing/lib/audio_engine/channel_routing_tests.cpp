#include <gtest/gtest.h>

import std;
import channel_routing;

using namespace audio_engine;

TEST(ChannelRouting, makeChannelRouting) {
    auto channelRoutingResult { audio_mixer::makeChannelRouting() };
    ASSERT_TRUE(channelRoutingResult.has_value());
    EXPECT_FALSE(channelRoutingResult.value()->m_leftMono.has_value());
    EXPECT_FALSE(channelRoutingResult.value()->m_right.has_value());
    EXPECT_FALSE(channelRoutingResult.value()->isMono());
    EXPECT_FALSE(channelRoutingResult.value()->isStereo());

    channelRoutingResult = audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 2 });
    ASSERT_TRUE(channelRoutingResult.has_value());
    ASSERT_TRUE(channelRoutingResult.value()->m_leftMono.has_value());
    EXPECT_EQ(channelRoutingResult.value()->m_leftMono, 2);
    EXPECT_FALSE(channelRoutingResult.value()->m_right.has_value());
    EXPECT_TRUE(channelRoutingResult.value()->isMono());
    EXPECT_FALSE(channelRoutingResult.value()->isStereo());

    channelRoutingResult = audio_mixer::makeChannelRouting(std::nullopt, audio_mixer::Routing_t { 2 });
    ASSERT_FALSE(channelRoutingResult.has_value());
    EXPECT_EQ(channelRoutingResult, std::unexpected { std::string { "Channels must be either left/mono or stereo" } });

    channelRoutingResult = audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 3 }, audio_mixer::Routing_t { 4 });
    ASSERT_TRUE(channelRoutingResult.has_value());
    ASSERT_TRUE(channelRoutingResult.value()->m_leftMono.has_value());
    EXPECT_EQ(channelRoutingResult.value()->m_leftMono, 3);
    ASSERT_TRUE(channelRoutingResult.value()->m_right.has_value());
    EXPECT_EQ(channelRoutingResult.value()->m_right.value(), 4);
    EXPECT_FALSE(channelRoutingResult.value()->isMono());
    EXPECT_TRUE(channelRoutingResult.value()->isStereo());

    channelRoutingResult = audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 65535 }, audio_mixer::Routing_t { 2 });
    ASSERT_FALSE(channelRoutingResult.has_value());
    EXPECT_EQ(channelRoutingResult, std::unexpected { std::string { "Invalid left/Mono channel ID" } });

    channelRoutingResult = audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 2 }, audio_mixer::Routing_t { 65535 });
    ASSERT_FALSE(channelRoutingResult.has_value());
    EXPECT_EQ(channelRoutingResult, std::unexpected { std::string { "Invalid right channel ID" } });
    
    channelRoutingResult = audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 2 }, audio_mixer::Routing_t { 2 });
    ASSERT_FALSE(channelRoutingResult.has_value());
    EXPECT_EQ(channelRoutingResult, std::unexpected { std::string { "Invalid channel routing" } });

    channelRoutingResult = audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 3 }, audio_mixer::Routing_t { 2 });
    ASSERT_FALSE(channelRoutingResult.has_value());
    EXPECT_EQ(channelRoutingResult, std::unexpected { std::string { "Invalid channel routing" } });

    channelRoutingResult = audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 3 }, audio_mixer::Routing_t { 5 });
    ASSERT_FALSE(channelRoutingResult.has_value());
    EXPECT_EQ(channelRoutingResult, std::unexpected { std::string { "Invalid channel routing" } });
}

TEST(ChannelRoutingSerializer, serializeDeserialize) {
    auto inputRouting { *audio_mixer::makeChannelRouting().value() };
    auto outputRouting { *audio_mixer::makeChannelRouting().value() };

    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::serialize(inputRouting), audio_mixer::SerializedRouting_t { 0b00000000000000000000000000000000 });
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::serializeInputOutput(inputRouting, outputRouting), audio_mixer::SerializedInputOutputRouting_t { 0b0000000000000000000000000000000000000000000000000000000000000000 });
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::deserialize(audio_mixer::SerializedRouting_t { 0b00000000000000000000000000000000 }), inputRouting);
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::deserializeInputOutput(audio_mixer::SerializedInputOutputRouting_t { 0b0000000000000000000000000000000000000000000000000000000000000000 }), std::make_pair(inputRouting, outputRouting));

    inputRouting.m_leftMono = audio_mixer::Routing_t { 0 };
    outputRouting.m_leftMono = audio_mixer::Routing_t { 0 };

    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::serialize(inputRouting), audio_mixer::SerializedRouting_t { 0b00000000000000010000000000000000 });
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::serializeInputOutput(inputRouting, outputRouting), audio_mixer::SerializedInputOutputRouting_t { 0b0000000000000001000000000000000000000000000000010000000000000000 });
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::deserialize(audio_mixer::SerializedRouting_t { 0b00000000000000010000000000000000 }), inputRouting);
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::deserializeInputOutput(audio_mixer::SerializedInputOutputRouting_t { 0b0000000000000001000000000000000000000000000000010000000000000000 }), std::make_pair(inputRouting, outputRouting));

    inputRouting.m_leftMono = audio_mixer::Routing_t { 1 };
    outputRouting.m_leftMono = audio_mixer::Routing_t { 1 };

    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::serialize(inputRouting), audio_mixer::SerializedRouting_t { 0b00000000000000100000000000000000 });
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::serializeInputOutput(inputRouting, outputRouting), audio_mixer::SerializedInputOutputRouting_t { 0b0000000000000010000000000000000000000000000000100000000000000000 });
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::deserialize(audio_mixer::SerializedRouting_t { 0b00000000000000100000000000000000 }), inputRouting);
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::deserializeInputOutput(audio_mixer::SerializedInputOutputRouting_t { 0b0000000000000010000000000000000000000000000000100000000000000000 }), std::make_pair(inputRouting, outputRouting));

    inputRouting.m_leftMono = audio_mixer::Routing_t { audio_mixer::MAX_CHANNEL_ID };
    outputRouting.m_leftMono = audio_mixer::Routing_t { audio_mixer::MAX_CHANNEL_ID };

    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::serialize(inputRouting), audio_mixer::SerializedRouting_t { 0b11111111111111110000000000000000 });
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::serializeInputOutput(inputRouting, outputRouting), audio_mixer::SerializedInputOutputRouting_t { 0b1111111111111111000000000000000011111111111111110000000000000000 });
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::deserialize(audio_mixer::SerializedRouting_t { 0b11111111111111110000000000000000 }), inputRouting);
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::deserializeInputOutput(audio_mixer::SerializedInputOutputRouting_t { 0b1111111111111111000000000000000011111111111111110000000000000000 }), std::make_pair(inputRouting, outputRouting));

    inputRouting.m_right = audio_mixer::Routing_t { 0 };
    outputRouting.m_right = audio_mixer::Routing_t { 0 };

    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::serialize(inputRouting), audio_mixer::SerializedRouting_t { 0b11111111111111110000000000000001 });
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::serializeInputOutput(inputRouting, outputRouting), audio_mixer::SerializedInputOutputRouting_t { 0b1111111111111111000000000000000111111111111111110000000000000001 });
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::deserialize(audio_mixer::SerializedRouting_t { 0b11111111111111110000000000000001 }), inputRouting);
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::deserializeInputOutput(audio_mixer::SerializedInputOutputRouting_t { 0b1111111111111111000000000000000111111111111111110000000000000001 }), std::make_pair(inputRouting, outputRouting));

    inputRouting.m_right = audio_mixer::Routing_t { 1 };
    outputRouting.m_right = audio_mixer::Routing_t { 1 };

    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::serialize(inputRouting), audio_mixer::SerializedRouting_t { 0b11111111111111110000000000000010 });
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::serializeInputOutput(inputRouting, outputRouting), audio_mixer::SerializedInputOutputRouting_t { 0b1111111111111111000000000000001011111111111111110000000000000010 });
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::deserialize(audio_mixer::SerializedRouting_t { 0b11111111111111110000000000000010 }), inputRouting);
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::deserializeInputOutput(audio_mixer::SerializedInputOutputRouting_t { 0b1111111111111111000000000000001011111111111111110000000000000010 }), std::make_pair(inputRouting, outputRouting));

    inputRouting.m_right = audio_mixer::Routing_t { audio_mixer::MAX_CHANNEL_ID };
    outputRouting.m_right = audio_mixer::Routing_t {  audio_mixer::MAX_CHANNEL_ID };

    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::serialize(inputRouting), audio_mixer::SerializedRouting_t { 0b11111111111111111111111111111111 });
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::serializeInputOutput(inputRouting, outputRouting), audio_mixer::SerializedInputOutputRouting_t { 0b1111111111111111111111111111111111111111111111111111111111111111 });
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::deserialize(audio_mixer::SerializedRouting_t { 0b11111111111111111111111111111111 }), inputRouting);
    EXPECT_EQ(audio_mixer::ChannelRoutingSerializer::deserializeInputOutput(audio_mixer::SerializedInputOutputRouting_t { 0b1111111111111111111111111111111111111111111111111111111111111111 }), std::make_pair(inputRouting, outputRouting));
}

TEST(ChannelRouting, makeRoutingList) {
    EXPECT_EQ(audio_mixer::makeRoutingList(0), std::vector<std::unique_ptr<audio_mixer::ChannelRouting>> {});

    std::vector<std::unique_ptr<audio_mixer::ChannelRouting>> expectedRoutingList {};
    expectedRoutingList.emplace_back(audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 0 }).value());
    expectedRoutingList.emplace_back(audio_mixer::makeChannelRouting().value());

    auto routingList { audio_mixer::makeRoutingList(1) };

    ASSERT_EQ(routingList.size(), expectedRoutingList.size());
    for (size_t i { 0 }; i < routingList.size(); ++i)
        EXPECT_EQ(*expectedRoutingList[i], *routingList[i]);

    expectedRoutingList.clear();
    expectedRoutingList.emplace_back(audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 0 }).value());
    expectedRoutingList.emplace_back(audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 1 }).value());
    expectedRoutingList.emplace_back(audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 2 }).value());
    expectedRoutingList.emplace_back(audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 0 }, audio_mixer::Routing_t { 1 }).value());
    expectedRoutingList.emplace_back(audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 1 }, audio_mixer::Routing_t { 2 }).value());
    expectedRoutingList.emplace_back(audio_mixer::makeChannelRouting().value());

    routingList = audio_mixer::makeRoutingList(3);

    ASSERT_EQ(routingList.size(), expectedRoutingList.size());
    for (size_t i { 0 }; i < routingList.size(); ++i)
        EXPECT_EQ(*expectedRoutingList[i], *routingList[i]);

    expectedRoutingList.clear();
    expectedRoutingList.emplace_back(audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 0 }).value());
    expectedRoutingList.emplace_back(audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 1 }).value());
    expectedRoutingList.emplace_back(audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 2 }).value());
    expectedRoutingList.emplace_back(audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 3 }).value());
    expectedRoutingList.emplace_back(audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 0 }, audio_mixer::Routing_t { 1 }).value());
    expectedRoutingList.emplace_back(audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 1 }, audio_mixer::Routing_t { 2 }).value());
    expectedRoutingList.emplace_back(audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 2 }, audio_mixer::Routing_t { 3 }).value());
    expectedRoutingList.emplace_back(audio_mixer::makeChannelRouting().value());

    routingList = audio_mixer::makeRoutingList(4);

    ASSERT_EQ(routingList.size(), expectedRoutingList.size());
    for (size_t i { 0 }; i < routingList.size(); ++i)
        EXPECT_EQ(*expectedRoutingList[i], *routingList[i]);
}

TEST(ChannelRouting, toString) {
    EXPECT_EQ(audio_mixer::toString(*audio_mixer::makeChannelRouting().value()), std::string { "No audio" });
    EXPECT_EQ(audio_mixer::toString(*audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 2 }).value()), std::string { "2" });
    EXPECT_EQ(audio_mixer::toString(*audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 10 }, audio_mixer::Routing_t { 11 }).value()), std::string { "10/11" });
}
