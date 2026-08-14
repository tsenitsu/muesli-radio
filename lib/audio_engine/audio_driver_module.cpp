module;
#include <miniaudio.h>
export module audio_driver;

import std;

namespace audio_engine::audio_driver {

export enum class AudioDriver {
#ifdef _WIN32
    Wasapi,
    DirectSound,
    WinMM,
#endif
#ifdef __APPLE__
    CoreAudio,
#endif
#ifdef __linux__
    PulseAudio,
    Jack,
    Alsa,
#endif
    Null
};

export [[nodiscard]] auto toString(AudioDriver driver) -> std::expected<std::string, std::string>;
export [[nodiscard]] auto toBackend(AudioDriver driver) -> std::expected<ma_device_backend_vtable*, std::string>;
export [[nodiscard]] auto toAudioDriver(const ma_device_backend_vtable* backend) -> std::expected<AudioDriver, std::string>;

export constexpr std::array availableAudioDrivers {
#ifdef _WIN32
    AudioDriver::Wasapi,
    AudioDriver::DirectSound,
    AudioDriver::WinMM,
#endif
#ifdef __APPLE__
    AudioDriver::CoreAudio,
#endif
#ifdef __linux__
    AudioDriver::PulseAudio,
    AudioDriver::Jack,
    AudioDriver::Alsa,
#endif
#ifndef NDEBUG
    AudioDriver::Null
#endif
};

}