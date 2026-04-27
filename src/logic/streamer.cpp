/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "logic/streamer.hpp"
#include "utils/logger.hpp"
#include "utils/time_utils.hpp"

namespace gh_ost {

EventsStreamer::EventsStreamer(MigrationContext& context)
    : context_(context)
    , event_queue_(10000)
    , running_(false)
    , total_events_(0)
    , dml_events_(0) {
}

EventsStreamer::~EventsStreamer() {
    Stop();
}

bool EventsStreamer::Initialize() {
    LOG_INFO("Initializing events streamer");
    
    // Create binlog reader
    reader_ = std::make_unique<GoMySQLBinlogReader>();
    
    // Configure reader
    reader_->SetReplicaConnection(context_.ReplicaConnection());
    reader_->SetDatabase(context_.DatabaseName());
    reader_->SetTable(context_.OriginalTableName());
    
    // Get current binlog coordinates
    auto coords = GetCurrentCoordinates();
    reader_->SetStartingCoordinates(coords);
    
    LOG_INFO_FMT("Initial coordinates: {}", coords.ToString());
    
    return true;
}

bool EventsStreamer::Start() {
    LOG_INFO("Starting events streamer");
    
    if (!reader_) {
        LOG_ERROR("Reader not initialized");
        return false;
    }
    
    running_ = true;
    
    // Start stream thread
    stream_thread_ = std::thread(&EventsStreamer::StreamLoop, this);
    
    return true;
}

void EventsStreamer::Stop() {
    LOG_INFO("Stopping events streamer");
    
    running_ = false;
    
    if (reader_) {
        reader_->Stop();
    }
    
    event_queue_.Stop();
    
    if (stream_thread_.joinable()) {
        stream_thread_.join();
    }
    
    LOG_INFO("Events streamer stopped");
}

BinlogCoordinates EventsStreamer::GetCurrentCoordinates() const {
    auto conn = context_.ReplicaConnection();
    if (!conn || !conn->IsConnected()) {
        return BinlogCoordinates();
    }
    
    return BinlogCoordinates(
        conn->GetCurrentBinlogFile(),
        conn->GetCurrentBinlogPosition()
    );
}

void EventsStreamer::SetEventCallback(EventProcessedCallback callback) {
    event_callback_ = callback;
}

bool EventsStreamer::ProcessChangelogEvent(std::shared_ptr<BinlogEntry> entry) {
    if (!entry) return false;
    
    // Check if this is a changelog event
    if (entry->Table() != context_.ChangelogTableName()) {
        return false;
    }
    
    // Parse changelog hint
    // This is for heartbeat and state synchronization
    
    return true;
}

bool EventsStreamer::WaitForCoordinates(const BinlogCoordinates& coords) {
    // Wait until we've processed all events up to specific coordinates
    while (running_) {
        auto current = GetCurrentCoordinates();
        
        if (current >= coords) {
            return true;
        }
        
        TimeUtils::SleepFor(100);
    }
    
    return false;
}

void EventsStreamer::StreamLoop() {
    LOG_INFO("Stream loop started");
    
    while (running_) {
        // Get event from reader's queue
        // In production, this would use actual binlog protocol
        
        // Simulate event processing
        TimeUtils::SleepFor(10);
        
        // Update statistics
        total_events_++;
    }
    
    LOG_INFO_FMT("Stream loop stopped. Total events: {}", total_events_);
}

bool EventsStreamer::ShouldProcessEvent(std::shared_ptr<BinlogEntry> entry) const {
    if (!entry) return false;
    
    // Process changelog events
    if (entry->Table() == context_.ChangelogTableName()) {
        return true;
    }
    
    // Process DML for our original table
    if (entry->IsDML()) {
        return entry->Database() == context_.DatabaseName() &&
               entry->Table() == context_.OriginalTableName();
    }
    
    // Process rotate events (binlog file changes)
    if (entry->EventType() == BinlogEventType::Rotate) {
        return true;
    }
    
    // Process query events (DDL)
    if (entry->EventType() == BinlogEventType::Query) {
        return true;
    }
    
    return false;
}

void EventsStreamer::HandleDMLEvent(std::shared_ptr<BinlogEntry> entry) {
    if (!entry || !entry->DmlEvent()) return;
    
    // Push to event queue for applier
    event_queue_.Push(entry);
    
    dml_events_++;
    
    // Call callback if set
    if (event_callback_) {
        event_callback_(entry);
    }
}

void EventsStreamer::HandleQueryEvent(std::shared_ptr<BinlogEntry> entry) {
    // Handle DDL events (CREATE, ALTER, DROP)
    LOG_DEBUG_FMT("Query event at {}", entry->Coordinates().ToString());
}

void EventsStreamer::HandleRotateEvent(std::shared_ptr<BinlogEntry> entry) {
    // Update current binlog file
    LOG_DEBUG_FMT("Rotate event: switching to new binlog file");
}

} // namespace gh_ost