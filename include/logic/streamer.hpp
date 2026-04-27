/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_STREAMER_HPP
#define GH_OST_STREAMER_HPP

#include "context/migration_context.hpp"
#include "binlog/binlog_reader.hpp"
#include "binlog/binlog_entry.hpp"
#include "utils/safe_queue.hpp"
#include <memory>
#include <atomic>
#include <thread>
#include <functional>

namespace gh_ost {

// Events streamer - manages binlog streaming and event filtering
class EventsStreamer {
public:
    EventsStreamer(MigrationContext& context);
    ~EventsStreamer();
    
    // Initialize
    bool Initialize();
    
    // Start/Stop streaming
    bool Start();
    void Stop();
    
    // Get current binlog position
    BinlogCoordinates GetCurrentCoordinates() const;
    
    // Event queue access
    SafeQueue<std::shared_ptr<BinlogEntry>>& GetEventQueue() { return event_queue_; }
    
    // Statistics
    uint64_t TotalEventsRead() const { return total_events_; }
    uint64_t DMLEventsProcessed() const { return dml_events_; }
    
    // Callback for processed events
    using EventProcessedCallback = std::function<void(std::shared_ptr<BinlogEntry>)>;
    void SetEventCallback(EventProcessedCallback callback);
    
    // Process changelog hints
    bool ProcessChangelogEvent(std::shared_ptr<BinlogEntry> entry);
    
    // Wait for all events up to specific coordinates
    bool WaitForCoordinates(const BinlogCoordinates& coords);
    
private:
    MigrationContext& context_;
    std::unique_ptr<BinlogReader> reader_;
    
    SafeQueue<std::shared_ptr<BinlogEntry>> event_queue_;
    
    std::atomic<bool> running_;
    std::atomic<uint64_t> total_events_;
    std::atomic<uint64_t> dml_events_;
    
    EventProcessedCallback event_callback_;
    
    std::thread stream_thread_;
    
    // Stream loop
    void StreamLoop();
    
    // Filter events
    bool ShouldProcessEvent(std::shared_ptr<BinlogEntry> entry) const;
    
    // Handle different event types
    void HandleDMLEvent(std::shared_ptr<BinlogEntry> entry);
    void HandleQueryEvent(std::shared_ptr<BinlogEntry> entry);
    void HandleRotateEvent(std::shared_ptr<BinlogEntry> entry);
};

} // namespace gh_ost

#endif // GH_OST_STREAMER_HPP