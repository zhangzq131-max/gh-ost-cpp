/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "utils/thread_pool.hpp"

namespace gh_ost {

ThreadPool::ThreadPool(size_t num_threads)
    : num_threads_(num_threads)
    , running_(false)
    , stopped_(true)
    , active_tasks_(0) {
}

ThreadPool::~ThreadPool() {
    StopAndWait();
}

void ThreadPool::Start() {
    if (running_) return;
    
    stopped_ = false;
    running_ = true;
    
    threads_.reserve(num_threads_);
    for (size_t i = 0; i < num_threads_; ++i) {
        threads_.emplace_back(&ThreadPool::WorkerThread, this);
    }
}

void ThreadPool::Stop() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        stopped_ = true;
        running_ = false;
    }
    condition_.notify_all();
}

void ThreadPool::StopAndWait() {
    Stop();
    
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threads_.clear();
    
    // Clear remaining tasks
    std::queue<std::function<void()>> empty;
    std::swap(tasks_, empty);
}

void ThreadPool::Restart() {
    StopAndWait();
    Start();
}

void ThreadPool::WorkerThread() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(mutex_);
            
            // Wait for task or stop signal
            condition_.wait(lock, [this]() {
                return stopped_ || !tasks_.empty();
            });
            
            if (stopped_ && tasks_.empty()) {
                return;
            }
            
            if (tasks_.empty()) continue;
            
            task = std::move(tasks_.front());
            tasks_.pop();
            active_tasks_++;
        }
        
        // Execute task outside of lock
        task();
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            active_tasks_--;
        }
        completed_.notify_all();
    }
}

void ThreadPool::WaitAll() {
    std::unique_lock<std::mutex> lock(mutex_);
    completed_.wait(lock, [this]() {
        return stopped_ || (tasks_.empty() && active_tasks_ == 0);
    });
}

void ThreadPool::SetNumThreads(size_t num_threads) {
    StopAndWait();
    num_threads_ = num_threads;
    Start();
}

size_t ThreadPool::PendingTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

size_t ThreadPool::ActiveTasks() const {
    return active_tasks_.load();
}

} // namespace gh_ost