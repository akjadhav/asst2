#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include "itasksys.h"
#include <mutex>
#include <atomic>
#include <queue>
#include <set>
#include <tuple>
#include <condition_variable>
#include <thread>
#include <unordered_map>
#include <memory>  // Added for std::shared_ptr

/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial: public ITaskSystem {
    public:
        TaskSystemSerial(int num_threads);
        ~TaskSystemSerial();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn: public ITaskSystem {
    public:
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
    
    private:
        void workerThread();
        std::vector<std::thread> threads;
        std::mutex mutex;
        int _curr_task;
        int _num_total_tasks;
        int numThreads;
        IRunnable* _runnable;
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
    
    private:
        void workerThread();
        int num_threads;
        std::vector<std::thread> threads;
        bool finished;
        std::mutex mutex;
        std::mutex completed_mutex;
        IRunnable* _runnable;
        int _tasksCompleted;
        int _tasksAssigned;
        int _num_total_tasks;
        int _currTask;
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
    
    private:
        void workerThread(int i);
        void addToReadyQueue(TaskID id);

        struct TaskGroup {
            std::shared_ptr<IRunnable> runnable; 
            int num_total_tasks;
            int completed_tasks;
            std::vector<TaskID> dependencies;
            std::vector<TaskID> successors;
            std::atomic<int> predecessors;
        };

        std::unordered_map<TaskID, TaskGroup> task_groups;
        std::queue<std::tuple<TaskID, int>> ready_queue;
        TaskID next_task_id;
        std::atomic<int> total_tasks_submitted;
        std::atomic<int> total_tasks_completed;

        std::vector<std::thread> threads;
        bool finished;

        std::mutex mutex;
        std::condition_variable worker_cv;
        std::condition_variable main_cv;
};

#endif
