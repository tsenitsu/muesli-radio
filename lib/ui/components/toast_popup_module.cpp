module;
#include <visage_ui/frame.h>
export module toast_popup;

import std;

namespace ui::components {

export class ToastPopup : public visage::Frame, public visage::EventTimer {
public:
    ToastPopup(std::string_view message, int durationMs);

    auto onDismiss(std::function<void(ToastPopup*)> callback) -> void;

    auto timerCallback() -> void override;
    auto draw(visage::Canvas& canvas) -> void override;
    auto mouseDown(const visage::MouseEvent& e) -> void override;
    auto mouseEnter(const visage::MouseEvent&) -> void override;
    auto mouseExit(const visage::MouseEvent&) -> void override;

private:
    auto dismiss() -> void;

    static constexpr int POLL_MS { 30 };

    std::string m_message;
    std::chrono::steady_clock::time_point m_startTime;
    int m_durationMs;
    bool m_isHovered;
    std::function<void(ToastPopup*)> m_onDismiss;
    float m_progress;
};

}