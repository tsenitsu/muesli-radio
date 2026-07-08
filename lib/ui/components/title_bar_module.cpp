module;
#include <visage_ui/frame.h>
export module title_bar;

import std;

namespace ui::components {

export class TitleBar : public visage::Frame {
public:
    explicit TitleBar(std::string_view title);

    auto draw(visage::Canvas& canvas) -> void override;

private:
    std::string m_title;
};

}