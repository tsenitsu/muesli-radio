#include <gtest/gtest.h>

import std;

import async_task_scheduler;
using namespace async_task_scheduler;

constexpr int numberOfTasks { 10 };
constexpr unsigned int numberOfThreads { 5 };

TEST(AsyncTaskScheduler, makeAsyncTaskScheduler) {
    auto asyncTaskScheduler { makeAsyncTaskScheduler(0) };
    EXPECT_FALSE(asyncTaskScheduler.has_value());
    EXPECT_EQ(asyncTaskScheduler, std::unexpected<std::string> { "Concurrency level must be greater than 0" });

    asyncTaskScheduler = makeAsyncTaskScheduler(numberOfTasks);
    EXPECT_EQ(asyncTaskScheduler.has_value(), true);
}

TEST(AsyncTaskScheduler, executeTasks) {
    ASSERT_TRUE(numberOfThreads < numberOfTasks);

    std::array<bool, numberOfTasks> tasksCompletion {};
    tasksCompletion.fill(false);

    auto scheduler { makeAsyncTaskScheduler(numberOfThreads) };

    std::vector<std::unique_ptr<AsyncTask>> tasks {};
    std::vector<Dependency> dependencies {};

    std::array<std::jthread, numberOfTasks> threads {};

    for (auto i { 0 }; i < numberOfTasks; ++i) {
        auto task { makeAtomicTask([&tasksCompletion, i] () { tasksCompletion[i] = true; }) };
        dependencies.emplace_back(task->dependency());
        tasks.emplace_back(std::move(task));
    }

    for (auto i { 0 }; i < numberOfTasks; ++i) {
        threads[i] = std::jthread { [&scheduler, task = std::move(tasks[i]), i] () mutable { scheduler.value()->enqueueTask(std::move(task), i); } };
    }

    for (auto& thread: threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    for (auto& dependency: dependencies) {
        dependency.wait();
    }

    for (auto i { 0 }; auto taskCompletion: tasksCompletion) {
        if (i++ < numberOfThreads)
            ASSERT_EQ(taskCompletion, true);
        else
            ASSERT_EQ(taskCompletion, false);
    }
}