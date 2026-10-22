module;
#include <visage_ui/frame.h>
export module label;

import std;

namespace ui::components {

export class Label: public visage::Frame {
public:
    explicit Label(std::string_view text);

    auto text(std::string_view text) -> void;
    auto draw(visage::Canvas& canvas) -> void override;

private:
    std::string m_text;
};

}

