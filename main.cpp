/*
 * This program starts a 10 seconds long audio stream by opening the default input and output devices on the system.
 * Beware the Larsen effect: use headphones!
 **/

import std;

import audio_engine;
using namespace audio_engine;

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

    auto taskManager { makeTaskManager(*asyncTaskScheduler) };

    std::unique_ptr<AudioEngine<audio_library_wrapper::MiniaudioLibraryWrapper>> audioEngine { nullptr };

    if (auto audioEngineResult { makeAudioEngine<audio_library_wrapper::MiniaudioLibraryWrapper>([] (const std::string& log) { std::print("{}", log); }) };
        not audioEngineResult.has_value()) {
        std::println("Cannot create audio engine: {}", audioEngineResult.error());
        return 1;
    } else {
        audioEngine.swap(audioEngineResult.value());
    }

    auto probeDevicesTask { makeAtomicTask([&audioEngine] () { return audioEngine->probeDevices(); }) };
    auto probeDevicesResult { probeDevicesTask->result() };

    taskManager->enqueueTasks(std::move(probeDevicesTask));

    if (auto result { probeDevicesResult.get() }; not result.has_value()) {
        std::println("Cannot probe devices: {}", result.error());
        return 1;
    }

    auto inputDeviceNameTask { makeAtomicTask([&audioEngine] () { return audioEngine->defaultInputAudioDeviceName(); }) };
    auto inputDeviceNameResult { inputDeviceNameTask->result() };

    taskManager->enqueueTasks(std::move(inputDeviceNameTask));

    std::string inputDeviceName { "" };
    if (auto result { inputDeviceNameResult.get() }; not result.has_value()) {
        std::println("Cannot get default input device name: {}", result.error());
        return 1;
    } else {
        inputDeviceName.swap(result.value());
    }

    auto outputDeviceNameTask { makeAtomicTask([&audioEngine] () { return audioEngine->defaultAudioOutputDeviceName(); }) };
    auto outputDeviceNameResult { outputDeviceNameTask->result() };

    taskManager->enqueueTasks(std::move(outputDeviceNameTask));

    std::string outputDeviceName { "" };
    if (auto result { outputDeviceNameResult.get() }; not result.has_value()) {
        std::println("Cannot get default output device name: {}", result.error());
        return 1;
    } else {
        outputDeviceName.swap(result.value());
    }

    auto startStreamTask { makeAtomicTask([&audioEngine, &inputDeviceName, &outputDeviceName] () { return audioEngine->startStream(inputDeviceName, outputDeviceName, 2048); }) } ;
    auto startStreamResult { startStreamTask->result() } ;

    taskManager->enqueueTasks(std::move(startStreamTask));

    if (auto result { startStreamResult.get() }; not result.has_value()) {
        std::println("Cannot start stream: {}", result.error());
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::seconds { 10 });
}
