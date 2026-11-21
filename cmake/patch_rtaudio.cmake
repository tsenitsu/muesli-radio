# Patches RtAudio after FetchContent downloads it (PATCH_COMMAND in lib/audio_engine/CMakeLists.txt). FetchContent runs
# this script in the RtAudio source directory.
#
# Why it is needed:
# RtAudio commit f0899de ("S24: change storage to 4 chars", after release 6.0.1) changed RTAUDIO_SINT24 from a packed
# 3-byte sample to a 24-bit sample in a 4-byte container, to match ALSA's SND_PCM_FORMAT_S24. The ASIO backend was not
# updated: RtApiAsio still maps the drivers' ASIOSTInt24LSB/MSB samples, which are packed in 3 bytes, to RTAUDIO_SINT24.
# With a 24-bit ASIO driver (e.g. Realtek ASIO) every callback copies bufferSize * 4 bytes per channel into, or out of,
# driver buffers of bufferSize * 3 bytes. The output copy writes past the driver's buffers and corrupts the heap (ASan:
# heap-buffer-overflow in RtApiAsio::callbackEvent), and the audio would be noise anyway because the samples are misaligned.
#
# The patch reverts f0899de, so RTAUDIO_SINT24 is a packed 3-byte sample again, as the ASIO backend expects. This is safe
# because the Windows build compiles only the ASIO backend. Remove the patch once RtApiAsio converts between RtAudio's
# 4-byte samples and the drivers' packed 3-byte buffers.
#
# The script can run more than once: a file that is already patched is left alone. If RtAudio changed the patched lines,
# configuration fails, so the patch is never silently skipped.

set(patchMarker "patched by muesli-radio: cmake/patch_rtaudio.cmake")

function(patch_rtaudio_file fileName oldText newText)
    set(filePath "${CMAKE_CURRENT_SOURCE_DIR}/${fileName}")

    if (NOT EXISTS "${filePath}")
        message(FATAL_ERROR "patch_rtaudio: ${filePath} not found. Is the script running in the RtAudio source directory?")
    endif()

    # file(READ) returns LF line endings even when the checkout has CRLF, so the texts below use \n only
    file(READ "${filePath}" content)

    string(FIND "${content}" "${newText}" newPosition)
    if (NOT newPosition EQUAL -1)
        message(STATUS "patch_rtaudio: ${fileName} is already patched")
        return()
    endif()

    string(FIND "${content}" "${oldText}" firstPosition)
    string(FIND "${content}" "${oldText}" lastPosition REVERSE)
    if (firstPosition EQUAL -1 OR NOT firstPosition EQUAL lastPosition)
        message(FATAL_ERROR "patch_rtaudio: the text to patch is not found exactly once in ${fileName}. RtAudio has changed: "
                            "check whether RtApiAsio now handles packed 24-bit samples, then update or remove the patch.\n"
                            "Text: ${oldText}")
    endif()

    string(REPLACE "${oldText}" "${newText}" content "${content}")
    file(WRITE "${filePath}" "${content}")
    message(STATUS "patch_rtaudio: patched ${fileName}")
endfunction()

# S24 holds a packed 3-byte sample again
patch_rtaudio_file(RtAudio.h
    "  unsigned char c3[4];"
    "  unsigned char c3[3]; // ${patchMarker}")

# formatBytes(RTAUDIO_SINT24) returns 3 again: RtApiAsio sizes its copies to and from the driver's buffers with it
patch_rtaudio_file(RtAudio.cpp
    "  else if ( format == RTAUDIO_SINT32 || format == RTAUDIO_FLOAT32 || format == RTAUDIO_SINT24)"
    "  else if ( format == RTAUDIO_SINT24 ) // ${patchMarker}\n    return 3;\n  else if ( format == RTAUDIO_SINT32 || format == RTAUDIO_FLOAT32 )")
