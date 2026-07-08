module;
#include <visage_ui/frame.h>
export module level_meter;

import std;

namespace ui::components {

export class LevelMeter : public visage::Frame, public visage::EventTimer {
public:
    explicit LevelMeter(unsigned int channelCount, std::function<std::span<const float>()> onTimerCallback, bool reversed = false);

    [[nodiscard]] auto channelCount() const -> unsigned int;

    auto onTimer(std::function<std::span<const float>()> callback) -> void;
    auto timerCallback() -> void override;
    auto draw(visage::Canvas& canvas) -> void override;

private:
    static constexpr auto REFRESH_RATE_MS { 20 };
    std::vector<float> m_visualLevels;
    bool m_reversed;
    std::function<std::span<const float>()> m_onTimer;
};

}