#include <gtest/gtest.h>

import std;
import async_task;
import resumable_task;

using namespace async_task_scheduler;

ResumableTask<void> task1() {
    co_await std::suspend_always {};
    co_await std::suspend_always {};
    co_await std::suspend_always {};
}

ResumableTask<void> task2() {
    co_await std::suspend_always {};
    co_await std::suspend_always {};
}

ResumableTask<void> task3() {
    co_await std::suspend_always {};
}

TEST(ResumableTask, testDependency) {
    const std::unique_ptr<AsyncTask> firstTask { makeResumableTask(task1()) };

    const std::unique_ptr<AsyncTask> secondTask { makeResumableTask(task2()) };
    secondTask->dependency(*firstTask);

    const std::unique_ptr<AsyncTask> thirdTask { makeResumableTask(task3()) };
    thirdTask->dependency(*firstTask);
    thirdTask->dependency(*secondTask);

    while (not firstTask->done()) {
        EXPECT_FALSE(secondTask->areDependenciesMet());
        EXPECT_FALSE(thirdTask->areDependenciesMet());

        EXPECT_FALSE(firstTask->done());
        EXPECT_FALSE(secondTask->done());
        EXPECT_FALSE(thirdTask->done());

        (*firstTask)();
    }

    EXPECT_TRUE(secondTask->areDependenciesMet());

    EXPECT_TRUE(firstTask->done());

    while (not secondTask->done()) {
        EXPECT_FALSE(thirdTask->areDependenciesMet());

        EXPECT_FALSE(secondTask->done());
        EXPECT_FALSE(thirdTask->done());

        (*secondTask)();
    }

    EXPECT_TRUE(thirdTask->areDependenciesMet());

    EXPECT_TRUE(secondTask->done());

    while (not thirdTask->done()) {
        EXPECT_FALSE(thirdTask->done());

        (*thirdTask)();
    }

    EXPECT_TRUE(thirdTask->done());
}

// Do not use references otherwise, when the coroutine is resumed after suspension, there will be no value
ResumableTask<std::string> makeString(std::string taskName, const int taskNumber) {
    co_await std::suspend_always {};
    co_return std::format("{} {}", taskName, taskNumber);
}

TEST(ResumableTask, result) {
    const auto taskCheckResult { makeResumableTask(makeString("Task", 1)) };
    const auto task1Result { taskCheckResult->result() };

    auto thread1 { std::jthread { std::jthread { [&] { while (not taskCheckResult->done()) (*taskCheckResult)(); } } } };
    EXPECT_EQ(task1Result.get(), "Task 1");
}