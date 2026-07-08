module;
#include <visage_ui/frame.h>
export module toggle_button;

import std;

namespace ui::components {

export class ToggleButton : public visage::Frame {
public:
    ToggleButton(std::string_view offText, std::string_view onText);

    auto toggleState(bool shouldBeOn, bool triggerCallback = true) -> void;
    auto enabled(bool shouldEnable) -> void;

    // Updated: Now expects a callback that returns a bool indicating success
    auto onToggle(std::function<bool(bool)> callback) -> void;

    auto draw(visage::Canvas& canvas) -> void override;
    auto mouseDown(const visage::MouseEvent& e) -> void override;
    auto mouseEnter(const visage::MouseEvent& e) -> void override;
    auto mouseExit(const visage::MouseEvent& e) -> void override;

private:
    std::string m_offText;
    std::string m_onText;
    bool m_isOn;
    bool m_isHovered;
    bool m_isEnabled;

    std::function<bool(bool)> m_onToggle;
};

}