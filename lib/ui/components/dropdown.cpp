module;
#include <visage_ui/frame.h>
module dropdown;

import colors;
import fonts;

namespace ui::components {

DropdownPopup::DropdownPopup(DropdownButton* owner, const float listX, const float listY, const float listW,
        std::vector<MenuItem> items, const unsigned int selectedId)
 :  m_owner { owner},
    m_listX { listX },
    m_listY { listY },
    m_listW { listW } ,
    m_items { std::move(items) },
    m_selectedId { selectedId },
    m_hoveredRow { std::numeric_limits<unsigned int>::max() } {}

auto DropdownPopup::draw(visage::Canvas& canvas) -> void {
    const float listH = static_cast<float>(m_items.size()) * ROW_H;

    canvas.setColor(colors::ComponentBackground);
    canvas.roundedRectangle(m_listX, m_listY, m_listW, listH, RADIUS);

    const visage::Font font(ROW_H * 0.45f, fonts::jetbrainsMonoRegular);
    for (size_t i = 0; i < m_items.size(); ++i) {
        const float rowY = m_listY + static_cast<float>(i) * ROW_H;

        if (m_items[i].id == m_selectedId || i == m_hoveredRow) {
            canvas.setColor(m_items[i].id == m_selectedId ? colors::DropDownSelection : colors::ComponentBackgroundHover);
            canvas.fill(m_listX + BORDER_W, rowY + 1.f, m_listW - (BORDER_W * 2.f), ROW_H - 1.f);
        }

        canvas.setColor(m_items[i].id == m_selectedId ? colors::ComponentTextPrimary : colors::ComponentTextSecondary);
        canvas.text(visage::String(m_items[i].label.c_str()), font, visage::Font::kLeft,
                    m_listX + PAD_X, rowY, m_listW - PAD_X * 2.f, ROW_H);
    }


    canvas.setColor(colors::DropDownBorderOpen);
    canvas.roundedRectangleBorder(m_listX, m_listY, m_listW, listH, RADIUS, BORDER_W);
}

auto DropdownPopup::mouseDown(const visage::MouseEvent& e) -> void {
    if (const float h { static_cast<float>(m_items.size()) * ROW_H }; e.position.x >= m_listX && e.position.x <= m_listX + m_listW &&
                                          e.position.y >= m_listY && e.position.y <= m_listY + h)
        return;

    m_owner->closePopup();
}

auto DropdownPopup::mouseMove(const visage::MouseEvent& e) -> void {
    const float x = e.position.x;
    const float y = e.position.y;
    const float listH = static_cast<float>(m_items.size()) * ROW_H;

    const bool inRect = x >= m_listX && x <= m_listX + m_listW &&
                        y >= m_listY && y <= m_listY + listH;

    setCursorStyle(inRect ? visage::MouseCursor::Pointing : visage::MouseCursor::Arrow);

    if (const unsigned int row = inRect ? static_cast<unsigned int>((y - m_listY) / ROW_H) : std::numeric_limits<unsigned int>::max(); row != m_hoveredRow) {
        m_hoveredRow = row;
        redraw();
    }
}

auto DropdownPopup::mouseExit(const visage::MouseEvent&) -> void {
    setCursorStyle(visage::MouseCursor::Arrow);
    m_hoveredRow = std::numeric_limits<unsigned int>::max();
    redraw();
}

auto DropdownPopup::mouseUp(const visage::MouseEvent& e) -> void {
    if (const float listH = static_cast<float>(m_items.size()) * ROW_H; !(e.position.x >= m_listX && e.position.x <= m_listX + m_listW &&
                                                                            e.position.y >= m_listY && e.position.y <= m_listY + listH))
        return;

    if (const int row = static_cast<int>((e.position.y - m_listY) / ROW_H); row >= 0 && row < static_cast<int>(m_items.size())) {
        const auto idx = static_cast<size_t>(row);
        m_owner->popupSelected(m_items[idx].id, m_items[idx].label);
    }
}

DropdownButton::DropdownButton(std::string_view text)
 :  m_text { text },
    m_defaultText { text },
    m_onSelection { nullptr },
    m_onMenuOpen { nullptr },
    m_items { std::vector<MenuItem>{} },
    m_selectedId { std::numeric_limits<unsigned int>::max() },
    m_menuOpen { false },
    m_hovered { false },
    m_enabled { true },
    m_popup { nullptr } {}

auto DropdownButton::text(std::string_view text) -> void {
    m_text = text;
    redraw();
}

auto DropdownButton::items(std::vector<MenuItem>&& items) -> void {
    m_items = std::move(items);
    redraw();
}

auto DropdownButton::selectedId(const unsigned int id) -> void {
    m_selectedId = id;
    redraw();
}

auto  DropdownButton::enabled(const bool shouldEnable) -> void {
    m_enabled = shouldEnable;

    if (!m_enabled) {
        if (m_menuOpen) closePopup();
        m_hovered = false;
    }

    redraw();
}

auto DropdownButton::onSelection(std::function<void(unsigned int, std::string_view)> onSelection) -> void {
    m_onSelection = std::move(onSelection);
}

auto DropdownButton::onMenuOpen(std::function<std::vector<MenuItem>()> onMenuOpen) -> void {
    m_onMenuOpen = std::move(onMenuOpen);
}

auto DropdownButton::popupSelected(const unsigned int id, std::string_view label) -> void {
    m_selectedId = id;
    m_text = label;

    closePopup();

    if (m_onSelection)
        m_onSelection(id, label);
}

auto DropdownButton::closePopup() -> void {
    if (!m_popup)
        return;

    m_popup->setVisible(false);
    m_menuOpen = false;

    redraw();
}

auto DropdownButton::reset() -> void {
    m_items.clear();
    m_text = m_defaultText;
    m_selectedId = std::numeric_limits<unsigned int>::max();

    if (m_popup) {
        if (auto* top { topParentFrame() }) {
            top->removeChild(m_popup);
        }

        m_popup = nullptr;
    }

    m_menuOpen = false;
    redraw();
}

auto DropdownButton::draw(visage::Canvas& canvas) -> void {
    const float w { width() };
    const float h { height() };

    canvas.setColor(m_enabled ? (m_hovered ? colors::ComponentBackgroundHover : colors::ComponentBackground) : colors::ComponentBackgroundDisabled);
    canvas.roundedRectangle(0, 0, w, h, 4.f);

    canvas.setColor(m_enabled ? (m_menuOpen ? colors::DropDownBorderOpen : colors::ComponentBorder) : colors::ComponentBorderDisabled);
    canvas.roundedRectangleBorder(0, 0, w, h, 4.f, 1.f);

    canvas.setColor(m_enabled ? colors::ComponentTextPrimary : colors::ComponentDisabledText);

    const visage::Font font { h * 0.45f, fonts::jetbrainsMonoRegular };
    canvas.text(visage::String(m_text.c_str()), font, visage::Font::kLeft, 8.f, 0.f, w - h - 8.f, h);

    const float cx { w - (h * 0.5f) };
    const float cy { h * 0.5f };
    constexpr float aw { 8.f };
    constexpr float ah { 4.f };

    canvas.setColor(m_enabled ? (m_hovered ? colors::ComponentTextPrimary : colors::ComponentTextSecondary) : colors::DropDownDisabledChevron);

    if (m_menuOpen) {
        canvas.segment(cx - aw/2, cy + ah/2, cx, cy - ah/2, 1.5f, false);
        canvas.segment(cx, cy - ah/2, cx + aw/2, cy + ah/2, 1.5f, false);
    } else {
        canvas.segment(cx - aw/2, cy - ah/2, cx, cy + ah/2, 1.5f, false);
        canvas.segment(cx, cy + ah/2, cx + aw/2, cy - ah/2, 1.5f, false);
    }
}

auto DropdownButton::mouseEnter(const visage::MouseEvent&) -> void {
    if (m_enabled) {
        m_hovered = true;
        setCursorStyle(visage::MouseCursor::Pointing);
        redraw();
    }
}

auto DropdownButton::mouseExit (const visage::MouseEvent&) -> void {
    if (m_enabled) {
        setCursorStyle(visage::MouseCursor::Arrow);
    }

    m_hovered = false;
    redraw();
}

auto DropdownButton::mouseDown(const visage::MouseEvent&) -> void {
    if (!m_enabled)
        return;

    if (m_menuOpen) {
        closePopup();
        return;
    }

    if (m_popup) {
        if (auto* top = topParentFrame()) top->removeChild(m_popup);
        m_popup = nullptr;
    }

    m_menuOpen = true;

    if (m_onMenuOpen) {
        items(m_onMenuOpen());
    }

    auto* top { topParentFrame() };
    const visage::Bounds b { top->relativeBounds(this) };

    auto popup { std::make_unique<DropdownPopup>(this, b.x(), b.y() + b.height(), b.width(), m_items, m_selectedId) };
    m_popup = popup.get();
    top->addChild(std::move(popup));

    m_popup->setOnTop(true);
    m_popup->setBounds(top->bounds());

    // Popup needs to be created otherwise button will always be selected
    if (m_items.empty()) {
        closePopup();
        return;
    }

    redraw();
}

}