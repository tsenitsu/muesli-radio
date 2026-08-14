module;
#include <miniaudio.h>
module audio_driver;

namespace audio_engine::audio_driver {

auto toString(const AudioDriver driver) -> std::expected<std::string, std::string> {
    return toBackend(driver)
            .and_then([] (ma_device_backend_vtable* backend) -> std::expected<std::string, std::string> {
                ma_device_backend_info info;
                ma_get_device_backend_info(backend, &info);

                if (info.pName == nullptr)
                    return std::unexpected("Failed to retrieve backend name");

                return std::string { info.pName };
                }
            );
}

auto toBackend(const AudioDriver driver) -> std::expected<ma_device_backend_vtable*, std::string>  {
    switch (driver) {
#ifdef _WIN32
        case AudioDriver::Wasapi:       return ma_device_backend_wasapi;
        case AudioDriver::DirectSound:  return ma_device_backend_dsound;
        case AudioDriver::WinMM:        return ma_device_backend_winmm;
#endif
#ifdef __APPLE__
        case AudioDriver::CoreAudio:    return ma_device_backend_coreaudio;
#endif
#ifdef __linux__
        case AudioDriver::PulseAudio:   return ma_device_backend_pulseaudio;
        case AudioDriver::Jack:         return ma_device_backend_jack;
        case AudioDriver::Alsa:         return ma_device_backend_alsa;
#endif
        case AudioDriver::Null:        return ma_device_backend_null;
        default: break;
    }

    return std::unexpected { "Audio driver unknown" };
}

auto toAudioDriver(const ma_device_backend_vtable* backend) -> std::expected<AudioDriver, std::string>  {
#ifdef _WIN32
        if (backend == ma_device_backend_wasapi)     return AudioDriver::Wasapi;
        if (backend == ma_device_backend_dsound)     return AudioDriver::DirectSound;
        if (backend == ma_device_backend_winmm)      return AudioDriver::WinMM;
#endif
#ifdef __APPLE__
        if (backend == ma_device_backend_coreaudio)  return AudioDriver::CoreAudio;
#endif
#ifdef __linux__
        if (backend == ma_device_backend_pulseaudio) return AudioDriver::PulseAudio;
        if (backend == ma_device_backend_jack)       return AudioDriver::Jack;
        if (backend == ma_device_backend_alsa)       return AudioDriver::Alsa;
#endif
        if (backend == ma_device_backend_null)       return AudioDriver::Null;

    return std::unexpected { "Audio backend unknown" };
}

}