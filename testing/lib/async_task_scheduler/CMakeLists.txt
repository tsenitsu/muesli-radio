add_executable(
        async-task-scheduler-unit-tests
        atomic_task_tests.cpp
        task_executor_tests.cpp
        async_task_scheduler_tests.cpp
        resumable_task_tests.cpp
)

target_link_libraries(
        async-task-scheduler-unit-tests PRIVATE
        GTest::gtest_main
        async-task-scheduler
)

gtest_discover_tests(async-task-scheduler-unit-tests)