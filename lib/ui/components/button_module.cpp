module;
#include <visage_ui/frame.h>
export module button;

import std;
import colors;
import fonts;

namespace ui::components {

export class Button : public visage::Frame {
public:
    explicit Button(std::string_view text);

    auto enabled(bool shouldEnable) -> void;
    auto onClick(std::function<void()> callback) -> void;

    auto draw(visage::Canvas& canvas) -> void override;
    auto mouseDown(const visage::MouseEvent& e) -> void override;
    auto mouseUp(const visage::MouseEvent& e) -> void override;
    auto mouseEnter(const visage::MouseEvent& e) -> void override;
    auto mouseExit(const visage::MouseEvent& e) -> void override;

private:
    std::string m_text;
    bool m_isHovered;
    bool m_isPressed;
    bool m_isEnabled;
    std::function<void()> m_onClick;
};

}