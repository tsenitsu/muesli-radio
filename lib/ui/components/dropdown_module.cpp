module;
#include <visage_ui/frame.h>
export module dropdown;

import std;

namespace ui::components {

export struct MenuItem {
    unsigned int id;
    std::string label;
};

export class DropdownButton;

class DropdownPopup : public visage::Frame {
public:
    DropdownPopup(DropdownButton* owner, float listX, float listY, float listW,
                  std::vector<MenuItem> items, unsigned int selectedId);

    auto draw(visage::Canvas& canvas) -> void override;
    auto mouseDown(const visage::MouseEvent& e) -> void override;
    auto mouseMove(const visage::MouseEvent& e) -> void override;
    auto mouseExit(const visage::MouseEvent&) -> void override;
    auto mouseUp(const visage::MouseEvent& e) -> void override;

private:
    static constexpr auto ROW_H { 32.f };
    static constexpr auto PAD_X { 12.f };
    static constexpr auto RADIUS { 4.f };
    static constexpr auto BORDER_W { 1.f };

    DropdownButton* m_owner;
    float m_listX;
    float m_listY;
    float m_listW;
    std::vector<MenuItem> m_items;
    unsigned int m_selectedId;
    unsigned int m_hoveredRow;
};

class DropdownButton : public visage::Frame {
public:
    explicit DropdownButton(std::string_view text);

    auto text(std::string_view text) -> void;
    auto items(std::vector<MenuItem>&& items) -> void;
    auto selectedId(unsigned int id) -> void;
    auto enabled(bool shouldEnable) -> void;
    auto onSelection(std::function<void(unsigned int, std::string_view)> onSelection) -> void;
    auto onMenuOpen(std::function<std::vector<MenuItem>()> onMenuOpen) -> void;

    auto popupSelected(unsigned int id, std::string_view label) -> void;
    auto closePopup() -> void;
    auto reset() -> void;

    auto draw(visage::Canvas& canvas) -> void override;
    auto mouseEnter(const visage::MouseEvent&) -> void override;
    auto mouseExit(const visage::MouseEvent&) -> void override;
    auto mouseDown(const visage::MouseEvent& e) -> void override;

private:
    std::string m_text;
    std::string m_defaultText;
    std::function<void(unsigned int, std::string_view)> m_onSelection;
    std::function<std::vector<MenuItem>()> m_onMenuOpen;
    std::vector<MenuItem> m_items;
    unsigned int m_selectedId;
    bool m_menuOpen;
    bool m_hovered;
    bool m_enabled;
    DropdownPopup* m_popup;
};

}