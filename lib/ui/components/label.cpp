module;
#include <visage_ui/frame.h>
module label;

import fonts;
import colors;

namespace ui::components {

Label::Label(std::string_view text)
 :  m_text { text } {}

auto Label::draw(visage::Canvas &canvas) -> void {
    canvas.setColor(colors::TextColor);
    const visage::Font font { height() / 2, fonts::jetbrainsMonoRegular };
    canvas.text(m_text, font, visage::Font::kLeft, 0, 0, width(), height());
}

}
