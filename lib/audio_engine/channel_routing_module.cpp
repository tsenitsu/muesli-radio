module;
#include <cinttypes>
export module channel_routing;

import std;

namespace audio_engine::audio_mixer {

export using Routing_t = uint16_t;
export using SerializedRouting_t = uint32_t;
export using SerializedInputOutputRouting_t = uint64_t;

// Serialized 0 means no input/output, so 1 is channel 0
export constexpr Routing_t MAX_CHANNEL_ID { std::numeric_limits<Routing_t>::max() - 1 };

export struct ChannelRouting final {
    explicit ChannelRouting(const std::optional<Routing_t>& leftMono = std::nullopt, const std::optional<Routing_t>& right = std::nullopt);

    std::optional<Routing_t> m_leftMono;
    std::optional<Routing_t> m_right;

    [[nodiscard]] auto isMono() const -> bool;
    [[nodiscard]] auto isStereo() const -> bool;

    auto operator==(const ChannelRouting& other) const -> bool = default;
};

export struct ChannelRoutingSerializer {
    // We assume that routing has already been validated upon construction
    [[nodiscard]] static auto serialize(const ChannelRouting& channelRouting) -> SerializedRouting_t;
    [[nodiscard]] static auto serializeInputOutput(const ChannelRouting& inputRouting, const ChannelRouting& outputRouting) -> SerializedInputOutputRouting_t;

    [[nodiscard]] static auto deserialize(SerializedRouting_t serializedRouting) -> ChannelRouting;
    [[nodiscard]] static auto deserializeInputOutput(SerializedInputOutputRouting_t serializedInputOutputRouting) -> std::pair<ChannelRouting, ChannelRouting>;
};

export [[nodiscard]] auto makeChannelRouting(const std::optional<Routing_t>& leftMono = std::nullopt, const std::optional<Routing_t>& right = std::nullopt) -> std::expected<std::unique_ptr<ChannelRouting>, std::string>;
export [[nodiscard]] auto makeRoutingList(Routing_t numberOfChannels) -> std::vector<std::unique_ptr<ChannelRouting>>;
export [[nodiscard]] auto toString(const ChannelRouting& channelRouting) -> std::string;

[[nodiscard]] auto isChannelRoutingValid(const std::optional<Routing_t>& leftMono, const std::optional<Routing_t>& right) -> std::expected<void, std::string>;

}