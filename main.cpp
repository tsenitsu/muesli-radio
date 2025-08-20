/*
 * This program starts a 10 seconds long audio stream by opening the default input and output devices on the system.
 * Beware the Larsen effect: use headphones!
 **/

import std;

import audio_engine;
using namespace audio_engine;

auto main() -> int {
    std::unique_ptr<AudioEngine<audio_library_wrapper::MiniaudioLibraryWrapper>> audioEngine { nullptr };

    if (auto audioEngineResult { audio_engine::makeAudioEngine<audio_library_wrapper::MiniaudioLibraryWrapper>([] (const std::string& log) { std::print("{}", log); }) };
        not audioEngineResult.has_value()) {
        std::println("Cannot create audio engine: {}", audioEngineResult.error());
        return 1;
    } else {
        audioEngine.swap(audioEngineResult.value());
    }

    if (auto probeDevicesResult { audioEngine->probeDevices() }; not probeDevicesResult.has_value()) {
        std::println("Cannot probe devices: {}", probeDevicesResult.error());
        return 1;
    }

    std::string inputDeviceName { "" };
    if (auto inputDeviceNameResult { audioEngine->defaultInputAudioDeviceName() }; not inputDeviceNameResult.has_value()) {
        std::println("Cannot get default input device name: {}", inputDeviceNameResult.error());
        return 1;
    } else {
        inputDeviceName.swap(inputDeviceNameResult.value());
    }

    std::string outputDeviceName { "" };
    if (auto outputDeviceNameResult { audioEngine->defaultAudioOutputDeviceName() }; not outputDeviceNameResult.has_value()) {
        std::println("Cannot get default output device name: {}", outputDeviceNameResult.error());
        return 1;
    } else {
        outputDeviceName.swap(outputDeviceNameResult.value());
    }

    if (auto startStreamResult { audioEngine->startStream(inputDeviceName, outputDeviceName, 2048) }; not startStreamResult.has_value()) {
        std::println("Cannot start stream: {}", startStreamResult.error());
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::seconds { 10 });
}
