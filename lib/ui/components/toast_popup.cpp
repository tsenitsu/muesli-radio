module;
#include <visage_ui/frame.h>
module toast_popup;

import colors;
import fonts;

namespace ui::components {
ToastPopup::ToastPopup(std::string_view message, int durationMs)
 :  m_message(message),
    m_startTime { std::chrono::steady_clock::now() },
    m_durationMs { durationMs },
    m_isHovered { false },
    m_onDismiss { nullptr },
    m_progress { 0.f } {
    startTimer(POLL_MS);
}

auto ToastPopup::onDismiss(std::function<void(ToastPopup*)> callback) -> void {
    m_onDismiss = std::move(callback);
}

auto ToastPopup::timerCallback() -> void {
    const auto elapsed { std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - m_startTime).count() };

    m_progress = std::min(1.0f, static_cast<float>(elapsed) / static_cast<float>(m_durationMs));

    if (elapsed >= m_durationMs) {
        stopTimer();
        dismiss();
    } else {
        redraw();

        if (auto* parentComponent = parent()) {
            parentComponent->redraw();
        }
    }
}

auto ToastPopup::draw(visage::Canvas& canvas) -> void {
    const float w { width() };
    const float h { height() };

    // 1. Draw Background & Border
    canvas.setColor(m_isHovered ? colors::ComponentBackgroundHover : colors::ComponentBackground);
    canvas.roundedRectangle(0, 0, w, h, 4.f);

    canvas.setColor(colors::ComponentBorder);
    canvas.roundedRectangleBorder(0, 0, w, h, 4.f, 1.f);

    // 2. Draw Progress Bar
    canvas.setColor(visage::Color(1.0f, 1.0f, 1.0f, 1.0f));
    const float barWidth { w * (1.0f - m_progress) };
    canvas.fill(0, 0, barWidth, 2.0f);

    // 3. FIX: Draw the 'X' using the rock-solid font engine instead of segments
    canvas.setColor(m_isHovered ? colors::ComponentTextPrimary : colors::ComponentTextSecondary);
    const visage::Font closeFont(h * 0.50f, fonts::jetbrainsMonoRegular);

    // This centers the "×" multiplication glyph perfectly inside the (0, 0, h, h) square boundary
    canvas.text(visage::String("×"), closeFont, visage::Font::kCenter, 0.f, 0.f, h, h);

    // 4. Draw Message Text
    canvas.setColor(colors::ComponentTextPrimary);
    const visage::Font font(h * 0.45f, fonts::jetbrainsMonoRegular);
    canvas.text(visage::String(m_message.c_str()), font, visage::Font::kLeft, h + 5.f, 0.f, w - h - 10.f, h);
}

auto ToastPopup::dismiss() -> void {
    if (m_onDismiss)
        m_onDismiss(this);
}

auto ToastPopup::mouseDown(const visage::MouseEvent&) -> void {
    setCursorStyle(visage::MouseCursor::Arrow);
    dismiss();
}

auto ToastPopup::mouseEnter(const visage::MouseEvent&) -> void {
    m_isHovered = true;
    setCursorStyle(visage::MouseCursor::Pointing);
    redraw();
}

auto ToastPopup::mouseExit(const visage::MouseEvent&) -> void {
    m_isHovered = false;
    setCursorStyle(visage::MouseCursor::Arrow);
    redraw();
}

}