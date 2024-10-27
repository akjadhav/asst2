#include "tasksys.h"
#include <thread>
#include <iostream>
#include <atomic>

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char *TaskSystemSerial::name()
{
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads) : ITaskSystem(num_threads)
{
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable *runnable, int num_total_tasks)
{
    for (int i = 0; i < num_total_tasks; i++)
    {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                                          const std::vector<TaskID> &deps)
{
    // You do not need to implement this method.
    return 0;
}

void TaskSystemSerial::sync()
{
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char *TaskSystemParallelSpawn::name()
{
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads) : ITaskSystem(num_threads)
{
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    this->num_threads = num_threads;
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable *runnable, int num_total_tasks)
{

    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    this->nextTaskId.store(0);
    auto worker_function = [&]()
    {
        while (true)
        {
            int task_id = this->nextTaskId.fetch_add(1);
            if (task_id >= num_total_tasks)
            {
                break;
            }
            runnable->runTask(task_id, num_total_tasks);
        }
    };
    std::thread workers[num_threads];
    for (int i = 0; i < num_threads; i++)
    {
        // int tasksPerThread = num_total_tasks / num_threads;
        // int start = tasksPerThread * i;
        // int end = (num_total_tasks * (i + 1)) / num_threads;
        // workers[i] = std::thread(worker_function, start, end);
        workers[i] = std::thread(worker_function);
    }
    for (auto &thread : workers)
    {
        thread.join();
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                                                 const std::vector<TaskID> &deps)
{
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelSpawn::sync()
{
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char *TaskSystemParallelThreadPoolSpinning::name()
{
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads) : ITaskSystem(num_threads)
{
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    this->destructorCalled = false;
    this->workers = new std::thread[num_threads];
    this->num_threads = num_threads;
    for (int i = 0; i < num_threads; i++)
    {
        this->workers[i] = std::thread(&TaskSystemParallelThreadPoolSpinning::workerFunction, this);
    }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning()
{
    this->destructorCalled = true;
    for (int i = 0; i < num_threads; ++i)
    {
        this->workers[i].join();
    }
    delete[] this->workers;
}

void TaskSystemParallelThreadPoolSpinning::workerFunction()
{
    while (true)
    {
        if (this->destructorCalled.load())
        {
            break;
        }
        std::unique_lock<std::mutex> lock(mutex);
        if (this->taskId.load() < this->num_total_tasks.load())
        {
            int curTask = this->taskId.load();
            this->taskId.fetch_add(1);
            lock.unlock();
            this->runnable.load()->runTask(curTask, this->num_total_tasks);
            this->completedTasks.fetch_add(1);
        }
    }
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable *runnable, int num_total_tasks)
{

    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    this->runnable.store(runnable);
    this->completedTasks.store(0);
    this->taskId.store(0);
    this->num_total_tasks.store(num_total_tasks);
    while (this->completedTasks.load() < this->num_total_tasks.load())
    {
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                                                              const std::vector<TaskID> &deps)
{
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync()
{
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char *TaskSystemParallelThreadPoolSleeping::name()
{
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads) : ITaskSystem(num_threads)
{
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    this->destructorCalled = false;
    this->workers = new std::thread[num_threads];
    this->num_threads = num_threads;
    for (int i = 0; i < num_threads; i++)
    {
        this->workers[i] = std::thread(&TaskSystemParallelThreadPoolSleeping::workerFunction, this);
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping()
{
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    this->destructorCalled = true;
    worker_cond.notify_all();
    for (int i = 0; i < num_threads; ++i)
    {
        this->workers[i].join();
    }
    delete[] this->workers;
}

void TaskSystemParallelThreadPoolSleeping::workerFunction()
{
    while (true)
    {
        if (this->destructorCalled.load())
        {
            break;
        }
        std::unique_lock<std::mutex> worker_thread_lock(worker_mutex);
        worker_cond.wait(worker_thread_lock, [this]
                         { return this->num_total_tasks > 0 || this->destructorCalled; });
        if (this->taskId < this->num_total_tasks)
        {
            int curTask = this->taskId;
            this->taskId = this->taskId + 1;
            worker_thread_lock.unlock();
            this->runnable->runTask(curTask, this->num_total_tasks);
            this->completedTasks.fetch_add(1);
        }
        if (this->completedTasks.load() >= this->num_total_tasks)
        {
            main_cond.notify_all();
        }
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable *runnable, int num_total_tasks)
{

    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    // add a lock to update these at the same time
    std::unique_lock<std::mutex> main_thread_lock(main_mutex);
    this->runnable = runnable;
    this->completedTasks.store(0);
    this->taskId = 0;
    this->num_total_tasks = num_total_tasks;
    // new tasks are available
    worker_cond.notify_all();
    while (this->completedTasks.load() < this->num_total_tasks)
    {
        main_cond.wait(main_thread_lock);
    }
    this->num_total_tasks = 0;
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                                                              const std::vector<TaskID> &deps)
{

    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync()
{

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
