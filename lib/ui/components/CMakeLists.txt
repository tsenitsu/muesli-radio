add_library(components
        title_bar.cpp
        main_window.cpp
        label.cpp
        dropdown.cpp
        toggle_button.cpp
        toast_popup.cpp
        level_meter.cpp
        button.cpp
)

target_custom_compiler_options(components)

target_sources(components
        PUBLIC
        FILE_SET cxx_modules
        TYPE CXX_MODULES
        FILES title_bar_module.cpp
        main_window_module.cpp
        label_module.cpp
        dropdown_module.cpp
        toggle_button_module.cpp
        toast_popup_module.cpp
        level_meter_module.cpp
        button_module.cpp
)

target_link_libraries(components PRIVATE visage colors fonts)

