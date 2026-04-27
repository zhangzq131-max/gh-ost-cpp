/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_SAFE_QUEUE_HPP
#define GH_OST_SAFE_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <atomic>
#include <chrono>

namespace gh_ost {

// Thread-safe queue for producer-consumer patterns
template<typename T>
class SafeQueue {
public:
    explicit SafeQueue(size_t max_size = 0);  // 0 = unlimited
    
    // Push element (blocking if queue is full)
    void Push(const T& item);
    void Push(T&& item);
    
    // Push element (non-blocking, returns false if queue is full)
    bool TryPush(const T& item);
    bool TryPush(T&& item);
    
    // Pop element (blocking)
    T Pop();
    
    // Pop element (non-blocking)
    std::optional<T> TryPop();
    
    // Pop with timeout
    std::optional<T> PopFor(uint64_t timeout_millis);
    std::optional<T> PopFor(std::chrono::milliseconds timeout);
    
    // Peek (non-blocking)
    std::optional<T> Peek();
    
    // Queue status
    bool Empty() const;
    bool Full() const;
    size_t Size() const;
    size_t MaxSize() const;
    
    // Queue operations
    void Clear();
    void SetMaxSize(size_t max_size);
    
    // Control
    void Stop();
    void Start();
    bool IsRunning() const { return running_; }
    
private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    size_t max_size_;
    std::atomic<bool> running_;
};

// Implementation
template<typename T>
SafeQueue<T>::SafeQueue(size_t max_size) 
    : max_size_(max_size), running_(true) {}

template<typename T>
void SafeQueue<T>::Push(const T& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    while (running_ && max_size_ > 0 && queue_.size() >= max_size_) {
        not_full_.wait(lock);
    }
    
    if (!running_) return;
    
    queue_.push(item);
    not_empty_.notify_one();
}

template<typename T>
void SafeQueue<T>::Push(T&& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    while (running_ && max_size_ > 0 && queue_.size() >= max_size_) {
        not_full_.wait(lock);
    }
    
    if (!running_) return;
    
    queue_.push(std::move(item));
    not_empty_.notify_one();
}

template<typename T>
bool SafeQueue<T>::TryPush(const T& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!running_ || (max_size_ > 0 && queue_.size() >= max_size_)) {
        return false;
    }
    
    queue_.push(item);
    not_empty_.notify_one();
    return true;
}

template<typename T>
bool SafeQueue<T>::TryPush(T&& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!running_ || (max_size_ > 0 && queue_.size() >= max_size_)) {
        return false;
    }
    
    queue_.push(std::move(item));
    not_empty_.notify_one();
    return true;
}

template<typename T>
T SafeQueue<T>::Pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    while (running_ && queue_.empty()) {
        not_empty_.wait(lock);
    }
    
    if (!running_ && queue_.empty()) {
        throw std::runtime_error("Queue is stopped and empty");
    }
    
    T item = std::move(queue_.front());
    queue_.pop();
    not_full_.notify_one();
    return item;
}

template<typename T>
std::optional<T> SafeQueue<T>::TryPop() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!running_ || queue_.empty()) {
        return std::nullopt;
    }
    
    T item = std::move(queue_.front());
    queue_.pop();
    not_full_.notify_one();
    return item;
}

template<typename T>
std::optional<T> SafeQueue<T>::PopFor(uint64_t timeout_millis) {
    return PopFor(std::chrono::milliseconds(timeout_millis));
}

template<typename T>
std::optional<T> SafeQueue<T>::PopFor(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    if (!not_empty_.wait_for(lock, timeout, [this]() { 
        return !running_ || !queue_.empty(); 
    })) {
        return std::nullopt;
    }
    
    if (!running_ || queue_.empty()) {
        return std::nullopt;
    }
    
    T item = std::move(queue_.front());
    queue_.pop();
    not_full_.notify_one();
    return item;
}

template<typename T>
std::optional<T> SafeQueue<T>::Peek() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!running_ || queue_.empty()) {
        return std::nullopt;
    }
    
    return queue_.front();
}

template<typename T>
bool SafeQueue<T>::Empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

template<typename T>
bool SafeQueue<T>::Full() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return max_size_ > 0 && queue_.size() >= max_size_;
}

template<typename T>
size_t SafeQueue<T>::Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

template<typename T>
size_t SafeQueue<T>::MaxSize() const {
    return max_size_;
}

template<typename T>
void SafeQueue<T>::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!queue_.empty()) {
        queue_.pop();
    }
    not_full_.notify_all();
}

template<typename T>
void SafeQueue<T>::SetMaxSize(size_t max_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    max_size_ = max_size;
    if (max_size_ > 0 && queue_.size() < max_size_) {
        not_full_.notify_all();
    }
}

template<typename T>
void SafeQueue<T>::Stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    running_ = false;
    not_empty_.notify_all();
    not_full_.notify_all();
}

template<typename T>
void SafeQueue<T>::Start() {
    std::lock_guard<std::mutex> lock(mutex_);
    running_ = true;
}

} // namespace gh_ost

#endif // GH_OST_SAFE_QUEUE_HPP