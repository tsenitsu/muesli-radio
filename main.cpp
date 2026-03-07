/*
 * This program starts an audio stream by opening the default input and output devices on the system and records the audio on disk.
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

    auto changeRoutingTask { makeAtomicTask([&audioEngine] () {
        const auto stereoInputRouting { audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 0 }, audio_mixer::Routing_t { 1 }). value() };
        const auto stereoOutputRouting { audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 0 }, audio_mixer::Routing_t { 1 }). value() };

        audioEngine->audioMixer()->inputRouting(std::make_pair(*stereoInputRouting, *stereoOutputRouting), 0);
        audioEngine->audioMixer()->outputRouting(*stereoOutputRouting, 0);
    }) };

    auto changeRoutingResult { changeRoutingTask->result() } ;
    taskManager->enqueueTasks(std::move(changeRoutingTask));
    changeRoutingResult.get();

    auto startRecordingTask { makeAtomicTask([&audioEngine] () {
        return audioEngine->startRecording(audio_format::AudioFormat::SignedInt24);
    }) };

    auto startRecordingResult { startRecordingTask->result() } ;
    taskManager->enqueueTasks(std::move(startRecordingTask));

    if (auto result { startRecordingResult.get() }; not result.has_value()) {
        std::println("Can not start recording: {}", result.error());
        return 1;
    }

    std::println("Started recording");

    std::this_thread::sleep_for(std::chrono::seconds { 5 });

    auto writeToFileTask { makeAtomicTask([&audioEngine] () {
        return audioEngine->write();
    }) };

    auto writeToFileResult { writeToFileTask->result() };
    taskManager->enqueueTasks(std::move(writeToFileTask));

    if (auto result { writeToFileResult.get() }; not result) {
        std::println("Can not write to file");
        return 1;
    }

    std::println("Written to file");
}
