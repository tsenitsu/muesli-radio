#include <gtest/gtest.h>

import std;

import async_task;
import atomic_task;
import task_executor;
import dependency;

using namespace async_task_scheduler;

constexpr int numberOfTasks { 10 };

TEST(TaskExecutor, executeTasks) {
    std::array<bool, numberOfTasks> tasksCompletion {};
    tasksCompletion.fill(false);

    auto executor { makeTaskExecutor() };

    std::vector<std::unique_ptr<AsyncTask>> tasks {};
    std::vector<Dependency> dependencies {};

    std::array<std::jthread, numberOfTasks> threads {};

    for (auto i { 0 }; i < numberOfTasks; ++i) {
        auto task { makeAtomicTask([&tasksCompletion, i] () { tasksCompletion[i] = true; }) };
        dependencies.emplace_back(task->dependency());
        tasks.emplace_back(std::move(task));
    }

    for (auto i { 0 }; i < numberOfTasks; ++i) {
        threads[i] = std::jthread { [&executor, task = std::move(tasks[i])] () mutable {  executor->enqueueTask(std::move(task)); } };
    }

    for (auto& thread: threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    for (auto& dependency: dependencies) {
        dependency.wait();
    }

    for (auto taskCompletion: tasksCompletion) {
        ASSERT_EQ(taskCompletion, true);
    }
}