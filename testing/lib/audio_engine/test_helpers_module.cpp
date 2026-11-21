module;
#include <miniaudio.h>
export module test_helpers;

import audio_device;

export namespace test_helpers {

// Makes distinct device ids for tests: miniaudio's null backend identifies a device with a plain int.
// ma_device_id() zeroes the whole union, which matters because ma_device_id_equal() compares it byte by byte.
[[nodiscard]] auto makeDeviceId(const int value) -> audio_engine::audio_device::DeviceId {
    auto id { ma_device_id() };
    id.nullbackend = value;
    return audio_engine::audio_device::DeviceId { id };
}

}
