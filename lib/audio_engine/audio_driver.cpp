module;
#include <miniaudio.h>
module audio_driver;

namespace audio_engine::audio_driver {

auto toString(const AudioDriver driver) -> std::expected<std::string, std::string> {
    return toBackend(driver).and_then([] (const ma_backend backend) { return std::expected<std::string, std::string> { ma_get_backend_name(backend) }; });
}

auto fromString(std::string_view driver) -> std::expected<AudioDriver, std::string> {
    ma_backend backend {};
    if (ma_get_backend_from_name(driver.data(), &backend))
        return std::unexpected { std::string { "Audio backend unknown" } };

    return toAudioDriver(backend);
}

auto toBackend(const AudioDriver driver) -> std::expected<ma_backend, std::string>  {
    switch (driver) {
#ifdef _WIN32
        case AudioDriver::Wasapi:       return ma_backend_wasapi;
        case AudioDriver::DirectSound:  return ma_backend_dsound;
        case AudioDriver::WinMM:        return ma_backend_winmm;
#endif
#ifdef __APPLE__
        case AudioDriver::CoreAudio:    return ma_backend_coreaudio;
#endif
#ifdef __linux__
        case AudioDriver::PulseAudio:        return ma_backend_pulseaudio;
        case AudioDriver::Alsa:         return ma_backend_alsa;
        case AudioDriver::Jack:         return ma_backend_jack;
#endif
        case AudioDriver::Null:        return ma_backend_null;
        default: break;
    }

    return std::unexpected { "Audio driver unknown" };
}

auto toAudioDriver(const ma_backend backend) -> std::expected<AudioDriver, std::string>  {
    switch (backend) {
#ifdef _WIN32
        case ma_backend_wasapi:     return AudioDriver::Wasapi;
        case ma_backend_dsound:     return AudioDriver::DirectSound;
        case ma_backend_winmm:      return AudioDriver::WinMM;
#endif
#ifdef __APPLE__
        case ma_backend_coreaudio:  return AudioDriver::CoreAudio;
#endif
#ifdef __linux__
        case ma_backend_pulseaudio: return AudioDriver::PulseAudio;
        case ma_backend_alsa:       return AudioDriver::Alsa;
        case ma_backend_jack:       return AudioDriver::Jack;
#endif
        case ma_backend_null:     return AudioDriver::Null;
        default: break;
    }

    return std::unexpected { "Audio backend unknown" };
}

}