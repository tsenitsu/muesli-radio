module;
// This needs to be placed here:
// This is an MSVC bug with C++ modules interacting with
// lambda default member initializers in visage::Frame.
// When MSVC compiles a module implementation unit (module title_bar;),
// it trips over frame.h's inline lambda members being initialized
// with brace-init syntax containing this — something like:
// Inside frame.h, roughly:
// some_callback_ = decltype(some_callback_){ this };
// MSVC fails to resolve the lambda type construction in that context
// when the TU is a named module unit.
#include <visage_ui/frame.h>
module title_bar;

import colors;
import fonts;

namespace ui::components {

TitleBar::TitleBar(std::string_view title)
 :  m_title { title } {
    setIgnoresMouseEvents(true, true);
}

auto TitleBar::draw(visage::Canvas& canvas) -> void {
    canvas.setColor(colors::DarkBackgroundColor);
    canvas.rectangle(0, 0, width(), height());

    canvas.setColor(colors::TextColor);
    const visage::Font font { height() / 2, fonts::jetbrainsMonoRegular };
    canvas.text(m_title, font, visage::Font::kCenter, 0, 0, width(), height());
}

}