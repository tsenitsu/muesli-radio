import std;

import audio_engine_manager;

import logger;
import logger_manager;

import async_task_scheduler;
import task_manager;
using namespace async_task_scheduler;

import main_window;
import ui_manager;
using namespace ui;

auto main() -> int {
    std::expected<managers::AudioEngineManager*, std::string> result { nullptr };

    std::unique_ptr<managers::LoggerManager> loggerManager { nullptr };
    std::unique_ptr<AsyncTaskScheduler> asyncTaskScheduler { nullptr };
    std::unique_ptr<managers::AudioEngineManager> audioEngineManager { nullptr };

    if (auto asyncTaskSchedulerResult { makeAsyncTaskScheduler(4) }; not asyncTaskSchedulerResult.has_value()) {
        result = std::unexpected { asyncTaskSchedulerResult.error() };
    } else {
        asyncTaskScheduler.swap(asyncTaskSchedulerResult.value());
    }


    if (result.has_value()) {
        if (auto loggerManagerResult  { managers::makeLoggerManager(*asyncTaskScheduler) }; not loggerManagerResult.has_value()) {
            result = std::unexpected { std::move(loggerManagerResult.error()) };
        } else {
            loggerManager.swap(loggerManagerResult.value());
        }

        if (result.has_value()) {
            auto audioEngineManagerResult { managers::makeAudioEngineManager(
                *asyncTaskScheduler, *loggerManager
            ) };

            if (not audioEngineManagerResult.has_value()) {
                result = std::unexpected { std::move(audioEngineManagerResult.error()) };
            } else {
                audioEngineManager = std::move(audioEngineManagerResult.value());
                result = audioEngineManager.get();
            }
        }
    }

    const auto uiManager { managers::makeUiManager(std::move(result)) };
    uiManager->run();

    return 0;
}
