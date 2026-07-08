module;
#include <visage_ui/frame.h>
module level_meter;

import colors;

namespace ui::components {

LevelMeter::LevelMeter(const unsigned int channelCount, std::function<std::span<const float>()> onTimerCallback, const bool reversed)
 :  m_visualLevels(channelCount),
    m_reversed { reversed },
    m_onTimer { std::move(onTimerCallback) } {
    if (channelCount > 0) {
        startTimer(REFRESH_RATE_MS);
    }
}

auto LevelMeter::channelCount() const -> unsigned int {
    return static_cast<unsigned int>(m_visualLevels.size());
}

auto LevelMeter::onTimer(std::function<std::span<const float>()> callback) -> void {
    m_onTimer = std::move(callback);
}

auto LevelMeter::timerCallback() -> void {
    if (not m_onTimer)
        return;

    bool needsRedraw { false };

    const auto levels { m_onTimer() };

    if (levels.size() != m_visualLevels.size())
        return;

    for (size_t i { 0 }; i < levels.size(); ++i) {
        m_visualLevels[i] = std::max(levels[i], m_visualLevels[i]);

        // Cap the maximum stored level so the meter doesn't "hang"
        // infinitely after massive audio spikes. We use 1.02f so it
        // still triggers your (level > 1.0f) clipping color.
        m_visualLevels[i] = std::min(m_visualLevels[i], 1.02f);
    }

    for (auto& level: m_visualLevels) {
        if (level > 0.001f) {
            constexpr float decayStep { 0.015f };
            level = std::max(0.0f, level - decayStep);
            needsRedraw = true;
        }
    }

    if (needsRedraw)
        redraw();
}

auto LevelMeter::draw(visage::Canvas& canvas) -> void {
    const float w { width() };
    const float h { height() };

    if (m_visualLevels.empty())
        return;

    constexpr float pad { 1.0f };
    const float barH { (h - (pad * (static_cast<float>(m_visualLevels.size()) + 1))) / static_cast<float>(m_visualLevels.size()) };

    for (size_t i { 0 }; i < m_visualLevels.size(); ++i) {
        float y { pad + static_cast<float>(i) * (barH + pad) };
        const float level { m_visualLevels[i] };

        canvas.setColor(visage::Color(0.2f, 0.1f, 0.1f, 0.1f));
        canvas.roundedRectangle(pad, y, w - (pad * 2), barH, 1.0f);

        visage::Color color {};

        if (level > 1.0f)
            color = visage::Color(1.0f, 0.90f, 0.20f, 0.20f);
        else
            color = visage::Color(1.0f, 0.25f, 0.85f, 0.35f);

        canvas.setColor(color);

        const float usableW { w - (pad * 2) };
        float signalW { std::min(level, 1.0f) * usableW };
        float signalX {};

        if (m_reversed) {
            signalX = (w - pad) - signalW;
        } else {
            signalX = pad;
        }

        if (signalW > 0.0f)
            canvas.roundedRectangle(signalX, y, signalW, barH, 1.0f);
    }
}

}