module;
#include <visage_ui/frame.h>
module button;

namespace ui::components {

Button::Button(std::string_view text)
 :  m_text { text },
    m_isHovered { false },
    m_isPressed { false },
    m_isEnabled { true },
    m_onClick { nullptr } {}

auto Button::enabled(const bool shouldEnable) -> void {
    m_isEnabled = shouldEnable;

    if (!m_isEnabled) {
        m_isHovered = false;
        m_isPressed = false;
        setCursorStyle(visage::MouseCursor::Arrow);
    }

    redraw();
}

auto Button::onClick(std::function<void()> callback) -> void {
    m_onClick = std::move(callback);
}

auto Button::draw(visage::Canvas& canvas) -> void {
    const float w { width() };
    const float h { height() };

    if (!m_isEnabled) {
        canvas.setColor(colors::ComponentBackgroundDisabled);
    } else if (m_isPressed) {
        canvas.setColor(colors::DropDownBorderOpen);
    } else if (m_isHovered) {
        canvas.setColor(colors::ComponentBackgroundHover);
    } else {
        canvas.setColor(colors::ComponentBackground);
    }

    canvas.roundedRectangle(0, 0, w, h, 4.f);

    if (!m_isEnabled) {
        canvas.setColor(colors::ComponentBorderDisabled);
    } else {
        canvas.setColor(m_isPressed ? colors::DropDownBorderOpen : colors::ComponentBorder);
    }

    canvas.roundedRectangleBorder(0, 0, w, h, 4.f, 1.f);

    if (!m_isEnabled) {
        canvas.setColor(colors::ComponentDisabledText);
    } else {
        canvas.setColor(colors::ComponentTextPrimary);
    }

    const visage::Font font { h * 0.45f, fonts::jetbrainsMonoRegular };
    canvas.text(visage::String(m_text.c_str()), font, visage::Font::kCenter, 0.f, 0.f, w, h);
}

auto Button::mouseDown(const visage::MouseEvent&) -> void {
    if (m_isEnabled) {
        m_isPressed = true;
        redraw();
    }
}

auto Button::mouseUp(const visage::MouseEvent&) -> void {
    if (m_isEnabled && m_isPressed) {
        m_isPressed = false;
        if (m_onClick) {
            m_onClick();
        }

        redraw();
    }
}

auto Button::mouseEnter(const visage::MouseEvent&) -> void {
    if (m_isEnabled) {
        m_isHovered = true;
        setCursorStyle(visage::MouseCursor::Pointing);
        redraw();
    }
}

auto Button::mouseExit(const visage::MouseEvent&) -> void {
    if (m_isHovered || m_isPressed) {
        m_isHovered = false;
        m_isPressed = false;
        setCursorStyle(visage::MouseCursor::Arrow);
        redraw();
    }
}

}