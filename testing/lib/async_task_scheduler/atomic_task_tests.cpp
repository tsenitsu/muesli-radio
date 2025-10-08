#include <gtest/gtest.h>

import std;
import async_task;
import atomic_task;

using namespace async_task_scheduler;

std::string fakeFunction(int number) { return std::format("Fake Function called with number {}", number); }

class FakeClass {
public:
    explicit FakeClass(const int number) :  m_number(number) {}
    [[nodiscard]] auto doSomething(const int pow) const -> int { return static_cast<int>(std::pow(m_number, pow)); }

private:
    int m_number;
};

TEST(AtomicTask, createAtomicTask) {
    // Lambda returning void with no input args and no captures
    const auto task1 { makeAtomicTask([] (){}) };

    // Lambda returning int with no input args and no captures
    const auto task2 { makeAtomicTask([] () { return 3; }) };

    // Lambda returning void with one input arg and no captures
    const auto task3 { makeAtomicTask([] ([[maybe_unused]] int number) {}, 2) };

    // Lambda returning int with one input arg and no capture
    const auto task4 { makeAtomicTask([] (const int number) {return number; }, 4) };

    // Lambda returning string with two input args and no capture
    const auto task5 { makeAtomicTask([] (const std::string& str1, const std::string& str2) { return str1 + str2; }, "Task", " 5") };

    // Lambda returning string with three input args and no capture
    const auto task6 { makeAtomicTask([] (const std::string& str1, const std::string& str2, int number) { return std::format("{} {} {}", str1, str2, number); }, "Task", " number ", 6) };

    // Lambda returning int with one input arg and one capture
    const auto dep { task6->dependency() };
    const auto task7 { makeAtomicTask([&] (const int number) { dep.wait(); return number; }, 8) };

    // Free function taking one argument and returning string
    const auto task8 { makeAtomicTask(fakeFunction, 6)};

    // std::bind
    auto fakeClass { FakeClass { 6 } };
    const auto task9 { makeAtomicTask(std::bind(&FakeClass::doSomething, &fakeClass, std::placeholders::_1), 2) };

    // std::function
    const auto task10 { makeAtomicTask(std::function { fakeFunction }, 8) };

    (*task1)();
    (*task2)();
    (*task3)();
    (*task4)();
    (*task5)();
    (*task6)();
    (*task7)();
    (*task8)();
    (*task9)();
    (*task10)();
}

TEST(AtomicTask, testDependency) {
    // task1 -> task2 -> task3 |-> task4 --> task7 -> task8 -> task9
    //                         |-> task5 ---|         |
    //                         |                      |
    //                         |-> task6 --------------

    std::unique_ptr<AsyncTask> task1 { nullptr };
    std::unique_ptr<AsyncTask> task2 { nullptr };
    std::unique_ptr<AsyncTask> task3 { nullptr };
    std::unique_ptr<AsyncTask> task4 { nullptr };
    std::unique_ptr<AsyncTask> task5 { nullptr };
    std::unique_ptr<AsyncTask> task6 { nullptr };
    std::unique_ptr<AsyncTask> task7 { nullptr };
    std::unique_ptr<AsyncTask> task8 { nullptr };
    std::unique_ptr<AsyncTask> task9 { nullptr };

    task1 = makeAtomicTask([&] () {
        EXPECT_FALSE(task1->done());
        EXPECT_FALSE(task2->done());
        EXPECT_FALSE(task3->done());
        EXPECT_FALSE(task4->done());
        EXPECT_FALSE(task5->done());
        EXPECT_FALSE(task6->done());
        EXPECT_FALSE(task7->done());
        EXPECT_FALSE(task8->done());
        EXPECT_FALSE(task9->done());
    });

    task2 = makeAtomicTask([&] () {
        EXPECT_TRUE(task1->done());

        EXPECT_FALSE(task2->done());
        EXPECT_FALSE(task3->done());
        EXPECT_FALSE(task4->done());
        EXPECT_FALSE(task5->done());
        EXPECT_FALSE(task6->done());
        EXPECT_FALSE(task7->done());
        EXPECT_FALSE(task8->done());
        EXPECT_FALSE(task9->done());
    });

    task2->dependency(*task1);

    task3 = makeAtomicTask([&] () {
        EXPECT_TRUE(task1->done());
        EXPECT_TRUE(task2->done());

        EXPECT_FALSE(task3->done());
        EXPECT_FALSE(task4->done());
        EXPECT_FALSE(task5->done());
        EXPECT_FALSE(task6->done());
        EXPECT_FALSE(task7->done());
        EXPECT_FALSE(task8->done());
        EXPECT_FALSE(task9->done());
    });

    task3->dependency(*task2);

    task4 = makeAtomicTask([&] () {
        EXPECT_TRUE(task1->done());
        EXPECT_TRUE(task2->done());
        EXPECT_TRUE(task3->done());

        EXPECT_FALSE(task4->done());
        //
        //
        EXPECT_FALSE(task7->done());
        EXPECT_FALSE(task8->done());
        EXPECT_FALSE(task9->done());
    });

    task4->dependency(*task3);

    task5 = makeAtomicTask([&] () {
        EXPECT_TRUE(task1->done());
        EXPECT_TRUE(task2->done());
        EXPECT_TRUE(task3->done());
        //
        EXPECT_FALSE(task5->done());
        //
        EXPECT_FALSE(task7->done());
        EXPECT_FALSE(task8->done());
        EXPECT_FALSE(task9->done());
    });

    task5->dependency(*task3);

    task6 = makeAtomicTask([&] () {
        EXPECT_TRUE(task1->done());
        EXPECT_TRUE(task2->done());
        EXPECT_TRUE(task3->done());
        //
        //
        EXPECT_FALSE(task6->done());
        //
        EXPECT_FALSE(task8->done());
        EXPECT_FALSE(task9->done());
    });

    task6->dependency(*task3);

    task7 = makeAtomicTask([&] () {
        EXPECT_TRUE(task1->done());
        EXPECT_TRUE(task2->done());
        EXPECT_TRUE(task3->done());
        EXPECT_TRUE(task4->done());
        EXPECT_TRUE(task5->done());
        //
        EXPECT_FALSE(task7->done());
        EXPECT_FALSE(task8->done());
        EXPECT_FALSE(task9->done());
    });

    task7->dependency(*task4);
    task7->dependency(*task5);

    task8 = makeAtomicTask([&] () {
        EXPECT_TRUE(task1->done());
        EXPECT_TRUE(task2->done());
        EXPECT_TRUE(task3->done());
        EXPECT_TRUE(task4->done());
        EXPECT_TRUE(task5->done());
        EXPECT_TRUE(task6->done());
        EXPECT_TRUE(task7->done());

        EXPECT_FALSE(task8->done());
        EXPECT_FALSE(task9->done());
    });

    task8->dependency(*task7);
    task8->dependency(*task6);

    task9 = makeAtomicTask([&] () {
        EXPECT_TRUE(task1->done());
        EXPECT_TRUE(task2->done());
        EXPECT_TRUE(task3->done());
        EXPECT_TRUE(task4->done());
        EXPECT_TRUE(task5->done());
        EXPECT_TRUE(task6->done());
        EXPECT_TRUE(task7->done());
        EXPECT_TRUE(task8->done());

        EXPECT_FALSE(task9->done());
    });

    task9->dependency(*task8);

    auto dep9 { task9->dependency() };

    // std::move() or std::ref() or lambda, do not use std::jthread { task } because you will hit stack overflow
    auto thread9 { std::jthread { [&] { while (not task9->done()) (*task9)(); } } };
    auto thread8 { std::jthread { [&] { while (not task8->done()) (*task8)(); } } };
    auto thread7 { std::jthread { [&] { while (not task7->done()) (*task7)(); } } };
    auto thread6 { std::jthread { [&] { while (not task6->done()) (*task6)(); } } };
    auto thread5 { std::jthread { [&] { while (not task5->done()) (*task5)(); } } };
    auto thread4 { std::jthread { [&] { while (not task4->done()) (*task4)(); } } };
    auto thread3 { std::jthread { [&] { while (not task3->done()) (*task3)(); } } };
    auto thread2 { std::jthread { [&] { while (not task2->done()) (*task2)(); } } };
    auto thread1 { std::jthread { [&] { while (not task1->done()) (*task1)(); } } };

    dep9.wait();

    EXPECT_TRUE(task1->done());
    EXPECT_TRUE(task2->done());
    EXPECT_TRUE(task3->done());
    EXPECT_TRUE(task4->done());
    EXPECT_TRUE(task5->done());
    EXPECT_TRUE(task6->done());
    EXPECT_TRUE(task7->done());
    EXPECT_TRUE(task8->done());
    EXPECT_TRUE(task9->done());
}

TEST(AtomicTask, result) {
    const auto task1 { makeAtomicTask([] (const std::string& text, int number) { return std::format("{} {}", text, number); }, "Task", 1) };
    const auto task1Result { task1->result() };

    auto thread1 { std::jthread { std::move(*task1) } };
    EXPECT_EQ(task1Result.get(), "Task 1");
}