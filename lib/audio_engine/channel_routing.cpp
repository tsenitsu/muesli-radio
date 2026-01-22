module channel_routing;

namespace audio_engine::audio_mixer {

ChannelRouting::ChannelRouting(const std::optional<Routing_t>& leftMono, const std::optional<Routing_t>& right)
 :  m_leftMono { leftMono }, m_right { right } {}

auto ChannelRouting::isMono() const -> bool {
    return m_leftMono.has_value() and not m_right.has_value();
}

auto ChannelRouting::isStereo() const -> bool {
    return m_leftMono.has_value() and m_right.has_value();
}

auto ChannelRoutingSerializer::serialize(const ChannelRouting& channelRouting) -> SerializedRouting_t {
    const SerializedRouting_t leftMono { channelRouting.m_leftMono.has_value()? channelRouting.m_leftMono.value() + 1 : SerializedRouting_t { 0 } };
    const SerializedRouting_t right { channelRouting.m_right.has_value()? channelRouting.m_right.value() + 1: SerializedRouting_t { 0 } };

    return right | leftMono << sizeof(Routing_t) * 8;
}

auto ChannelRoutingSerializer::serializeInputOutput(const ChannelRouting& inputRouting, const ChannelRouting& outputRouting) -> SerializedInputOutputRouting_t {
    const SerializedInputOutputRouting_t serializedInputRouting { serialize(inputRouting) };
    const SerializedInputOutputRouting_t serializedOutputRouting { serialize(outputRouting) };

    return serializedOutputRouting | serializedInputRouting << sizeof(SerializedRouting_t) * 8;
}

auto ChannelRoutingSerializer::deserialize(const SerializedRouting_t serializedRouting) -> ChannelRouting {
    constexpr auto shift = sizeof(Routing_t) * 8;
    constexpr SerializedRouting_t mask = 0xFFFF;   // Lower 16 bits

    const SerializedRouting_t leftMono { serializedRouting >> shift };
    const SerializedRouting_t right { serializedRouting & mask };

    return ChannelRouting {
        leftMono ? std::make_optional(static_cast<Routing_t>(leftMono - 1)) : std::nullopt,
        right ? std::make_optional(static_cast<Routing_t>(right - 1)) : std::nullopt
    };
}

auto ChannelRoutingSerializer::deserializeInputOutput(const SerializedInputOutputRouting_t serializedInputOutputRouting) -> std::pair<ChannelRouting, ChannelRouting> {
    constexpr auto shift = sizeof(SerializedRouting_t) * 8;
    constexpr SerializedInputOutputRouting_t mask = 0xFFFFFFFF;  // Lower 32 bits

    const auto inputRouting { deserialize(static_cast<SerializedRouting_t>(serializedInputOutputRouting >> shift)) };
    const auto outputRouting { deserialize(static_cast<SerializedRouting_t>(serializedInputOutputRouting & mask)) };

    return std::make_pair(inputRouting, outputRouting);
}

auto makeChannelRouting(const std::optional<Routing_t>& leftMono, const std::optional<Routing_t>& right) -> std::expected<std::unique_ptr<ChannelRouting>, std::string> {
    if (auto channelRoutingResult { isChannelRoutingValid(leftMono, right) }; not channelRoutingResult.has_value())
        return std::unexpected { std::move(channelRoutingResult).error() };

    return std::make_unique<ChannelRouting>(leftMono, right);
}

auto makeRoutingList(const Routing_t numberOfChannels) -> std::vector<std::unique_ptr<ChannelRouting>> {
    if (numberOfChannels == 0) {
        return {};
    }

    std::vector<std::unique_ptr<ChannelRouting>> routingList(numberOfChannels * 2);

    for (auto channel { Routing_t { 0 } }; channel < numberOfChannels; ++channel) {
        routingList[channel] = makeChannelRouting(channel).value();
        routingList[numberOfChannels + channel] = makeChannelRouting(channel, static_cast<Routing_t>(channel + 1)).value();
    }

    // Replace last invalid value (e.g. if number of channels is 4, 3/4, with "No audio"
    *std::ranges::rbegin(routingList) = makeChannelRouting().value();

    return routingList;
}

auto toString(const ChannelRouting& channelRouting) -> std::string {
    if (not channelRouting.m_leftMono.has_value())
        return "No audio";

    return std::format("{}", channelRouting.m_leftMono.value()).append(channelRouting.m_right.has_value()? std::format("/{}", channelRouting.m_right.value()) : std::string { "" });
}

auto isChannelRoutingValid(const std::optional<Routing_t>& leftMono, const std::optional<Routing_t>& right) -> std::expected<void, std::string> {
    if (leftMono.has_value() and leftMono.value() > MAX_CHANNEL_ID)
        return std::unexpected { std::string { "Invalid left/Mono channel ID" } };

    if (right.has_value() and right.value() > MAX_CHANNEL_ID)
        return std::unexpected { std::string { "Invalid right channel ID" } };

    // Routing must be left/mono or 1/2 2/3, channels must be adjacent
    if (leftMono.has_value() and right.has_value()) {
        if (leftMono.value() > right.value() or right.value() - leftMono.value() != 1)
            return std::unexpected { std::string { "Invalid channel routing" } };
    } else if (right.has_value()) {
        return std::unexpected { std::string { "Channels must be either left/mono or stereo" } };
    }

    return {};
}

}
