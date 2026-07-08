module;
#include <visage_ui/frame.h>
module toggle_button;

import colors;
import fonts;

namespace ui::components {

ToggleButton::ToggleButton(std::string_view offText, std::string_view onText)
 :  m_offText(offText),
    m_onText(onText),
    m_isOn { false },
    m_isHovered { false },
    m_isEnabled { true },
    m_onToggle { nullptr } {}

auto ToggleButton::toggleState(const bool shouldBeOn, const bool triggerCallback) -> void {
    if (m_isOn == shouldBeOn) return;

    if (triggerCallback && m_onToggle) {
        if (const bool success = m_onToggle(shouldBeOn); !success) {
            return;
        }
    }

    m_isOn = shouldBeOn;
    redraw();
}

auto ToggleButton::enabled(const bool shouldEnable) -> void {
    m_isEnabled = shouldEnable;
    if (!m_isEnabled) {
        m_isHovered = false;
        setCursorStyle(visage::MouseCursor::Arrow);
    }

    redraw();
}

auto ToggleButton::onToggle(std::function<bool(bool)> callback) -> void {
    m_onToggle = std::move(callback);
}

auto ToggleButton::draw(visage::Canvas& canvas) -> void {
    const float w { width() };
    const float h { height() };

    if (!m_isEnabled) {
        canvas.setColor(colors::ComponentBackgroundDisabled);
    } else if (m_isHovered) {
        canvas.setColor(colors::ComponentBackgroundHover);
    } else {
        canvas.setColor(colors::ComponentBackground);
    }

    canvas.roundedRectangle(0, 0, w, h, 4.f);

    if (!m_isEnabled) {
        canvas.setColor(colors::ComponentBorderDisabled);
    } else {
        canvas.setColor(m_isOn ? colors::DropDownBorderOpen : colors::ComponentBorder);
    }

    canvas.roundedRectangleBorder(0, 0, w, h, 4.f, 1.f);

    const std::string& label = m_isOn ? m_onText : m_offText;

    if (!m_isEnabled) {
        canvas.setColor(colors::ComponentDisabledText);
    } else {
        canvas.setColor(colors::ComponentTextPrimary);
    }

    const visage::Font font { h * 0.45f, fonts::jetbrainsMonoRegular };
    canvas.text(visage::String(label.c_str()), font, visage::Font::kCenter, 0.f, 0.f, w, h);
}

auto ToggleButton::mouseDown(const visage::MouseEvent&) -> void {
    if (m_isEnabled)
        toggleState(!m_isOn);
}

auto ToggleButton::mouseEnter(const visage::MouseEvent&) -> void {
    if (m_isEnabled) {
        m_isHovered = true;
        setCursorStyle(visage::MouseCursor::Pointing);
        redraw();
    }
}

auto ToggleButton::mouseExit(const visage::MouseEvent&) -> void {
    m_isHovered = false;
    setCursorStyle(visage::MouseCursor::Arrow);
    redraw();
}

}