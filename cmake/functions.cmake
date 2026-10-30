function(target_custom_compiler_options TARGET)
    if(MSVC)
        target_compile_options(${TARGET} PRIVATE
                /W4 /WX /permissive-
                /Zc:__cplusplus /Zc:preprocessor /Zc:inline
                /external:anglebrackets /external:W0
                /w14242 /w14254 /w14287
                /w14263 /w14265 /w14296 /w14311
                /w14545 /w14546 /w14547 /w14549 /w14555
                /w14619 /w14640 /w14826 /w14905 /w14906 /w14928)
    else()
        # Common to GCC and Clang (C and C++)
        target_compile_options(${TARGET} PRIVATE
                -Werror -Wall -Wextra -pedantic
                -Wshadow -Wconversion -Wsign-conversion
                -Wcast-align -Wcast-qual -Wundef -Wformat=2
                -Wdouble-promotion -Wnull-dereference
                -Wimplicit-fallthrough -Wvla)

        # C++-only warnings, common to GCC and Clang
        set(custom_cxx_warnings
                -Wreorder
                -Wnon-virtual-dtor
                -Woverloaded-virtual
                -Wold-style-cast
                -Wzero-as-null-pointer-constant
                -Wextra-semi
                -Wsuggest-override
                -Wmismatched-tags
                -Wpessimizing-move
                -Wredundant-move)

        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            list(APPEND custom_cxx_warnings
                    -Wduplicated-cond
                    -Wduplicated-branches
                    -Wlogical-op
                    -Wuseless-cast
                    -Wcast-align=strict
                    -Walloca
                    -Wunused-macros
                    -Wmissing-noreturn
                    -Wnrvo)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            list(APPEND custom_cxx_warnings
                    -Wmove
                    -Wshadow-all
                    -Wconditional-uninitialized
                    -Wloop-analysis
                    -Wcomma
                    -Wunreachable-code)
        endif()

        target_compile_options(${TARGET} PRIVATE
                "$<$<COMPILE_LANGUAGE:CXX>:${custom_cxx_warnings}>")

        # Cheap bounds checks for std::vector, std::span, etc. (libstdc++ only,
        # harmless elsewhere). Useful in Debug too, so applied to all configs.
        target_compile_definitions(${TARGET} PRIVATE _GLIBCXX_ASSERTIONS)
    endif()
endfunction()