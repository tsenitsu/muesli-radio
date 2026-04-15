/*
 * This program starts an audio stream by opening the default input and output devices on the system and records the audio on disk.
 * Beware the Larsen effect: use headphones!
 **/

import std;

import audio_engine;
import audio_engine_manager;

import async_task_scheduler;
import task_manager;
using namespace async_task_scheduler;

auto main() -> int {
    std::unique_ptr<AsyncTaskScheduler> asyncTaskScheduler { nullptr };
    if (auto asyncTaskSchedulerResult { makeAsyncTaskScheduler(4) }; not asyncTaskSchedulerResult.has_value()) {
        std::println("Cannot create async task scheduler: {}", asyncTaskSchedulerResult.error());
        return 1;
    } else {
        asyncTaskScheduler.swap(asyncTaskSchedulerResult.value());
    }

    std::unique_ptr<managers::AudioEngineManager> audioEngineManager { nullptr };
    if (auto audioEngineManagerResult { managers::makeAudioEngineManager(*asyncTaskScheduler, [] (const std::string& log) { std::print("{}", log); }) };
        not audioEngineManagerResult.has_value()) {
        std::println("Cannot create audio engine: {}", audioEngineManagerResult.error());
        return 1;
    } else {
        audioEngineManager.swap(audioEngineManagerResult.value());
    }

    auto probeDevicesResult { audioEngineManager->probeDevices() };
    if (auto result { probeDevicesResult.get() }; not result.has_value()) {
        std::println("Cannot probe devices: {}", result.error());
        return 1;
    }

    auto inputDeviceNameResult { audioEngineManager->defaultInputAudioDeviceName() };

    std::string inputDeviceName { "" };
    if (auto result { inputDeviceNameResult.get() }; not result.has_value()) {
        std::println("Cannot get default input device name: {}", result.error());
        return 1;
    } else {
        inputDeviceName.swap(result.value());
    }

    auto outputDeviceNameResult { audioEngineManager->defaultOutputAudioDeviceName() };

    std::string outputDeviceName { "" };
    if (auto result { outputDeviceNameResult.get() }; not result.has_value()) {
        std::println("Cannot get default output device name: {}", result.error());
        return 1;
    } else {
        outputDeviceName.swap(result.value());
    }

    auto startStreamResult { audioEngineManager->startStream(inputDeviceName, outputDeviceName, 2048) };

    if (auto result { startStreamResult.get() }; not result.has_value()) {
        std::println("Cannot start stream: {}", result.error());
        return 1;
    }

    const auto stereoInputRouting { audio_engine::audio_mixer::makeChannelRouting(audio_engine::audio_mixer::Routing_t { 0 }, audio_engine::audio_mixer::Routing_t { 1 }).value() };
    const auto stereoOutputRouting { audio_engine::audio_mixer::makeChannelRouting(audio_engine::audio_mixer::Routing_t { 0 }, audio_engine::audio_mixer::Routing_t { 1 }).value() };

    audioEngineManager->inputChannelRouting(std::make_pair(*stereoInputRouting, *stereoOutputRouting), 0);
    audioEngineManager->outputChannelRouting(*stereoOutputRouting, 0);

    const auto startRecordingResult { audioEngineManager->startRecording(audio_engine::audio_format::AudioFormat::SignedInt24) };

    if (not startRecordingResult.has_value()) {
        std::println("Can not start recording: {}", startRecordingResult.error());
        return 1;
    }

    std::println("Started recording");

    std::cin.get();
    audioEngineManager->stopRecording();

    std::println("Written to file");
}
