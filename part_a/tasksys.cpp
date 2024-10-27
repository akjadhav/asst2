#include "tasksys.h"
#include <thread>


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
    // You do not need to implement this method.
    return 0;
}

void TaskSystemSerial::sync() {
    // You do not need to implement this method.
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

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads), current_task_id(0) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    this->num_threads = num_threads;
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::executeTasks(IRunnable* runnable, int num_total_tasks) {
    while (true) {
        int task_id = current_task_id++;
        // all tasks complete
        if (task_id >= num_total_tasks) {
            break;
        }
        runnable->runTask(task_id, num_total_tasks);
    }
}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

  /* 
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
    */

    // assign tasks dynamically:
    current_task_id = 0;
    std::thread threads[num_threads];

    for (int i = 0; i < num_threads; i++) {
        threads[i] = std::thread(&TaskSystemParallelSpawn::executeTasks, this, runnable, num_total_tasks);
    }

    for (int i = 0; i < num_threads; i++) {
        threads[i].join();
    }


    
}
        

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // You do not need to implement this method.
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
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //

    this->num_threads = num_threads;
    stop_threads = false;

    // thread pool construction:
    for (int i = 0; i < num_threads; i++) {
        this->threads.push_back(std::thread(&TaskSystemParallelThreadPoolSpinning::executeTasks, this));
    }


}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    stop_threads = true;
    for (int i = 0; i < num_threads; i++) {
        threads[i].join();
    }

}

void TaskSystemParallelThreadPoolSpinning::executeTasks() {
    while (!stop_threads) {


        std::unique_lock<std::mutex> lock(mutex);
        int cur_task_id = task_id;
        // only do a task if task_id is less than total tasks
        if (cur_task_id < num_total_tasks) {
            task_id++;
            lock.unlock();
            // execute task
            runnable->runTask(cur_task_id, num_total_tasks);
            tasks_remaining--;
        } else {
            lock.unlock();
        }
        

    }

}



void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    //
    
    // for (int i = 0; i < num_total_tasks; i++) {
    //     runnable->runTask(i, num_total_tasks);
    // }

    this->num_total_tasks = num_total_tasks;
    this->runnable = runnable;
    this->tasks_remaining = num_total_tasks;
    this->task_id = 0;
    while (this->tasks_remaining > 0) { 

    }
    

}


TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // You do not need to implement this method.
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

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    this->num_threads = num_threads;
    stop_threads = false;

    for (int i = 0; i < num_threads; i++) {
        this->threads.push_back(std::thread(&TaskSystemParallelThreadPoolSleeping::executeTasks, this));
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    std::unique_lock<std::mutex> lock(mutex);
    stop_threads = true;
    // have all threads exit
    thread_condition.notify_all();
    lock.unlock();

    for (int i = 0; i < num_threads; i++) {
        threads[i].join();
    }


}

void TaskSystemParallelThreadPoolSleeping::executeTasks() {
    while (true) {
        std::unique_lock<std::mutex> lock(mutex);
        thread_condition.wait(lock, [this]() {
            return stop_threads || task_id < num_total_tasks;
        });

        if (stop_threads && task_id >= num_total_tasks) {
            break;
        }

        // if task remaining:
        if (task_id < num_total_tasks) {
            int cur_task_id = task_id++;
            lock.unlock();
            // run task
            runnable->runTask(cur_task_id, num_total_tasks);
            tasks_remaining--;

            if (tasks_remaining == 0) {
                // std::lock_guard<std::mutex> main_lock(mutex);
                main_condition.notify_one();
            }
        }

    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    // for (int i = 0; i < num_total_tasks; i++) {
    //     runnable->runTask(i, num_total_tasks);
    // }
    {
        std::lock_guard<std::mutex> lock(mutex);
        this->runnable = runnable;
        this->num_total_tasks = num_total_tasks;
        this->tasks_remaining = num_total_tasks;
        this->task_id = 0;
        // notify tasks are available
        thread_condition.notify_all();
    }

    // wait until no tasks remaining 
    std::unique_lock<std::mutex> lock(mutex);
    main_condition.wait(lock, [this]() {
        return tasks_remaining == 0;
    });



}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
