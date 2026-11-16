#include <gtest/gtest.h>

import std;
import async_task;
import async_task_scheduler;
import task_manager;

using namespace async_task_scheduler;

namespace {

class CallbackTask final : public AsyncTask {
public:
    explicit CallbackTask(std::function<void()> body) : m_body { std::move(body) } {}

    auto operator()() -> void override {
        m_body();
        m_done = true;
        signalCompletion();
    }

    [[nodiscard]] auto done() const -> bool override { return m_done; }

private:
    std::function<void()> m_body;
    std::atomic<bool> m_done { false };
};

auto makeTask(std::function<void()> body) -> std::unique_ptr<CallbackTask> {
    return std::make_unique<CallbackTask>(std::move(body));
}

struct TestableTaskManager : TaskManager {
    using TaskManager::TaskManager;
    using TaskManager::waitForAllTasks;
};

class TaskManagerTest : public testing::Test {
protected:
    static constexpr unsigned int threads = 4;

    TaskManagerTest()
     :  scheduler { makeAsyncTaskScheduler(threads).value() },
        manager { *scheduler } {}

    // Tasks capture test locals by reference, so none may outlive the test body
    auto TearDown() -> void override { manager.waitForAllTasks(); }

    // The OS thread that executes worker index
    auto threadOf(unsigned int index) -> std::thread::id {
        std::thread::id id;
        auto task = makeTask([&id] { id = std::this_thread::get_id(); });
        const auto dependency = task->dependency();

        manager.enqueueTasks(index, std::move(task));
        dependency.wait();
        return id;
    }

    std::unique_ptr<AsyncTaskScheduler> scheduler;
    TestableTaskManager manager;
};

TEST_F(TaskManagerTest, EachIndexMapsToItsOwnStableThread) {
    std::set<std::thread::id> distinct;
    for (unsigned int i = 0; i < threads; ++i) {
        const auto first = threadOf(i);
        EXPECT_EQ(threadOf(i), first);
        distinct.insert(first);
    }
    EXPECT_EQ(distinct.size(), threads);
}

TEST_F(TaskManagerTest, PinnedTasksRunOnTheRequestedThread) {
    const auto expected = threadOf(2);

    std::vector<std::thread::id> ids(50);
    for (auto& id : ids)
        manager.enqueueTasks(2u, makeTask([&id] { id = std::this_thread::get_id(); }));
    manager.waitForAllTasks();

    for (const auto& id : ids)
        EXPECT_EQ(id, expected);
}

TEST_F(TaskManagerTest, OutOfRangeIndexFallsBackToRandomAndLosesNoTasks) {
    std::atomic<int> counter { 0 };
    auto count = [&counter] { return makeTask([&counter] { ++counter; }); };

    manager.enqueueTasks(threads, count(), count());
    manager.enqueueTasks(std::numeric_limits<unsigned int>::max(), count());
    manager.waitForAllTasks();

    EXPECT_EQ(counter, 3);
}

TEST_F(TaskManagerTest, RandomModeDrawsAThreadPerTask) {
    constexpr std::size_t calls = 100;
    std::vector<std::array<std::thread::id, 4>> ids(calls);

    for (auto& slots : ids) {
        auto record = [&slots](const std::size_t i) {
            return makeTask([&slots, i] { slots[i] = std::this_thread::get_id(); });
        };
        manager.enqueueTasks(record(0), record(1), record(2), record(3));
    }
    manager.waitForAllTasks();

    // A call that shared one draw between its tasks would give a uniform row.
    // Chance that all 100 rows are uniform by luck: (1/64)^100.
    const bool anyRowMixed = std::ranges::any_of(ids, [](const auto& slots) {
        return std::ranges::adjacent_find(slots, std::not_equal_to<>{}) != slots.end();
    });
    EXPECT_TRUE(anyRowMixed);
}

TEST_F(TaskManagerTest, WaitForAllTasksBlocksUntilTasksFinish) {
    std::atomic<bool> finished { false };
    manager.enqueueTasks(makeTask([&finished] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        finished = true;
    }));

    manager.waitForAllTasks();
    EXPECT_TRUE(finished);
}

TEST_F(TaskManagerTest, ConcurrentEnqueueFromManyThreadsRunsEveryTask) {
    constexpr int producers = 8;
    constexpr int perProducer = 200;
    std::atomic<int> counter { 0 };

    {
        std::vector<std::jthread> workers;
        for (int p = 0; p < producers; ++p) {
            workers.emplace_back([&, p] {
                for (int i = 0; i < perProducer; ++i) {
                    if (i % 2 == 0)
                        manager.enqueueTasks(makeTask([&counter] { ++counter; }));
                    else
                        manager.enqueueTasks(static_cast<unsigned int>(p) % threads, makeTask([&counter] { ++counter; }));
                }
            });
        }
    } // jthreads join here

    manager.waitForAllTasks();
    EXPECT_EQ(counter, producers * perProducer);
}

}