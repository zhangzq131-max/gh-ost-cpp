/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_THREAD_POOL_HPP
#define GH_OST_THREAD_POOL_HPP

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <memory>

namespace gh_ost {

// Thread pool for concurrent task execution
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();
    
    // Start/Stop
    void Start();
    void Stop();
    void StopAndWait();
    void Restart();
    
    // Submit task
    template<typename F, typename... Args>
    auto Submit(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        
        using ReturnType = typename std::invoke_result<F, Args...>::type;
        
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<ReturnType> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (stopped_) {
                throw std::runtime_error("ThreadPool is stopped");
            }
            tasks_.emplace([task]() { (*task)(); });
        }
        
        condition_.notify_one();
        return result;
    }
    
    // Wait for all tasks to complete
    void WaitAll();
    
    // Get/set number of threads
    size_t NumThreads() const { return num_threads_; }
    void SetNumThreads(size_t num_threads);
    
    // Status
    size_t PendingTasks() const;
    size_t ActiveTasks() const;
    bool IsRunning() const { return running_; }
    bool IsStopped() const { return stopped_; }
    
private:
    void WorkerThread();
    
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::condition_variable completed_;
    
    size_t num_threads_;
    std::atomic<bool> running_;
    std::atomic<bool> stopped_;
    std::atomic<size_t> active_tasks_;
};

} // namespace gh_ost

#endif // GH_OST_THREAD_POOL_HPP