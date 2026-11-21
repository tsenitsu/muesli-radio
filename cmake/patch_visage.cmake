# Patches visage after FetchContent downloads or updates it (PATCH_COMMAND in lib/CMakeLists.txt). FetchContent runs
# this script in the visage source directory.
#
# Why it is needed:
# Some visage headers declare helpers at namespace scope as `static`, which gives them internal linkage (TU-local).
# Templates and inline functions in the same headers use them. When those headers are included by a C++ module
# (the global module fragment of our *_module.cpp files), GCC rejects every module that would expose such a
# TU-local entity: "error: ... exposes TU-local entity ...". MSVC and Clang accept the same code.
# Replacing `static` with `inline` gives the helpers external linkage without changing what they do: the standard way
# to define functions and constants in a header.
#
# Visage is followed on main, so the patch is applied to whatever main currently holds:
# - a change that is already applied is left alone, so running the script again is harmless;
# - a change whose text is no longer in visage only produces a warning, because MSVC and Clang do not need the patch.
#   If GCC then fails with "exposes TU-local entity", update the list below.

set(patchMarker "/* muesli-radio: was static, see cmake/patch_visage.cmake */")

function(patch_visage_line fileName oldText newText)
    set(filePath "${CMAKE_CURRENT_SOURCE_DIR}/${fileName}")

    if (NOT EXISTS "${filePath}")
        message(WARNING "patch_visage: ${fileName} not found in visage, the patch for it was not applied")
        return()
    endif()

    # file(READ) returns LF line endings even when the checkout has CRLF
    file(READ "${filePath}" content)

    string(FIND "${content}" "${newText}" newPosition)
    if (NOT newPosition EQUAL -1)
        return()
    endif()

    string(FIND "${content}" "${oldText}" firstPosition)
    string(FIND "${content}" "${oldText}" lastPosition REVERSE)
    if (firstPosition EQUAL -1 OR NOT firstPosition EQUAL lastPosition)
        message(WARNING "patch_visage: '${oldText}' is not found exactly once in ${fileName}: visage has changed. "
                        "If GCC fails with \"exposes TU-local entity\", update cmake/patch_visage.cmake.")
        return()
    endif()

    string(REPLACE "${oldText}" "${newText}" content "${content}")
    file(WRITE "${filePath}" "${content}")
    message(STATUS "patch_visage: patched ${fileName}")
endfunction()

patch_visage_line(visage_graphics/shape_batcher.h
    "  static void submitBaseShapes("
    "  inline ${patchMarker} void submitBaseShapes(")
patch_visage_line(visage_graphics/shape_batcher.h
    "  static void submitShapes("
    "  inline ${patchMarker} void submitShapes(")
