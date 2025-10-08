add_library(async-task-scheduler task_executor.cpp async_task_scheduler.cpp)

target_sources(async-task-scheduler
        PUBLIC
        FILE_SET cxx_modules
        TYPE CXX_MODULES
        FILES atomic_task_module.cpp
        async_task_module.cpp
        task_executor_module.cpp
        async_task_scheduler_module.cpp
        resumable_task_module.cpp
        signal_module.cpp
        dependency_module.cpp
        result_module.cpp
        task_manager_module.cpp
)