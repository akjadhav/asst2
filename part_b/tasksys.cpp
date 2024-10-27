#include "tasksys.h"

#include <memory>  // Added for std::shared_ptr

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemSerial::sync() {
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

struct NoOpDeleter {
    void operator()(void const *) const {}
};

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads)
    : ITaskSystem(num_threads),
      next_task_id(1),
      total_tasks_submitted(0),
      total_tasks_completed(0),
      finished(false) {
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(&TaskSystemParallelThreadPoolSleeping::workerThread, this, i);
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    {
        std::unique_lock<std::mutex> lock(mutex);
        finished = true;
        worker_cv.notify_all();
    }
    for (auto& thread : threads) {
        thread.join();
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {
    std::vector<TaskID> no_deps;
    runAsyncWithDeps(runnable, num_total_tasks, no_deps);
    sync();
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {
    std::unique_lock<std::mutex> lock(mutex);

    TaskID task_id = next_task_id++;
    TaskGroup& task_group = task_groups[task_id];
    task_group.runnable = std::shared_ptr<IRunnable>(runnable, NoOpDeleter());
    task_group.num_total_tasks = num_total_tasks;
    task_group.completed_tasks = 0;
    task_group.dependencies = deps;
    task_group.predecessors = deps.size();

    total_tasks_submitted += num_total_tasks;

    if (deps.empty()) {
        for (int i = 0; i < num_total_tasks; ++i) {
            ready_queue.emplace(task_id, i);
        }
        worker_cv.notify_one();
    } else {
        for (TaskID dep_id : deps) {
            task_groups[dep_id].successors.push_back(task_id);
        }
    }

    return task_id;
}

void TaskSystemParallelThreadPoolSleeping::sync() {
    std::unique_lock<std::mutex> lock(mutex);
    main_cv.wait(lock, [this]() { return total_tasks_completed == total_tasks_submitted; });

    total_tasks_submitted = 0;
    total_tasks_completed = 0;
    task_groups.clear();
}

void TaskSystemParallelThreadPoolSleeping::workerThread(int i) {
    while (true) {
        std::tuple<TaskID, int> task;
        {
            std::unique_lock<std::mutex> lock(mutex);
            while (ready_queue.empty() && !finished) {
                worker_cv.wait(lock);
            }

            if (finished && ready_queue.empty()) {
                break;
            }

            if (!ready_queue.empty()) {
                task = ready_queue.front();
                ready_queue.pop();
            } else {
                continue;
            }
        }

        while (true) {
            TaskID task_id = std::get<0>(task);
            int task_index = std::get<1>(task);

            std::shared_ptr<IRunnable> runnable;
            int num_total_tasks;

            {
                std::unique_lock<std::mutex> lock(mutex);
                TaskGroup& task_group = task_groups[task_id];
                runnable = task_group.runnable;
                num_total_tasks = task_group.num_total_tasks;
            }

            runnable->runTask(task_index, num_total_tasks);

            int completed_tasks = ++task_groups[task_id].completed_tasks;

            int total_completed = ++total_tasks_completed;

            if (completed_tasks == task_groups[task_id].num_total_tasks) {
                std::unique_lock<std::mutex> lock(mutex);
                TaskGroup& task_group = task_groups[task_id];
                for (TaskID succ_id : task_group.successors) {
                    TaskGroup& succ_group = task_groups[succ_id];
                    int preds = --succ_group.predecessors;
                    if (preds == 0) {
                        for (int i = 0; i < succ_group.num_total_tasks; ++i) {
                            ready_queue.emplace(succ_id, i);
                        }
                        worker_cv.notify_one(); 
                    }
                }
            }

            if (total_completed == total_tasks_submitted) {
                std::unique_lock<std::mutex> lock(mutex);
                main_cv.notify_all();
            }

            {
                std::unique_lock<std::mutex> lock(mutex);
                if (!ready_queue.empty()) {
                    task = ready_queue.front();
                    ready_queue.pop();
                } else {
                    break; 
                }
            }
        }
    }
}

