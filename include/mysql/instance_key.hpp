/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#ifndef GH_OST_INSTANCE_KEY_HPP
#define GH_OST_INSTANCE_KEY_HPP

#include <string>
#include <cstdint>
#include <optional>
#include <functional>
#include <sstream>

namespace gh_ost {

// Represents a MySQL server instance (host:port)
class InstanceKey {
public:
    InstanceKey();
    InstanceKey(const std::string& host, uint16_t port);
    explicit InstanceKey(const std::string& uri);  // Parse from "host:port"
    
    // Getters
    std::string Host() const { return host_; }
    uint16_t Port() const { return port_; }
    
    // String representation
    std::string ToString() const;          // "host:port"
    std::string ToURI() const;             // "host:port"
    std::string ToDisplayString() const;   // "host:port" (for display)
    
    // Comparison
    bool IsSameAs(const InstanceKey& other) const;
    bool IsValid() const;
    
    // Parse from string
    static std::optional<InstanceKey> Parse(const std::string& str);
    
    // Default values
    static InstanceKey Empty() { return InstanceKey(); }
    static InstanceKey Localhost(uint16_t port = 3306) {
        return InstanceKey("127.0.0.1", port);
    }
    
    // Operators
    bool operator==(const InstanceKey& other) const;
    bool operator!=(const InstanceKey& other) const;
    bool operator<(const InstanceKey& other) const;
    
private:
    std::string host_;
    uint16_t port_;
};

// Map of instance keys for tracking multiple servers
class InstanceKeyMap {
public:
    InstanceKeyMap();
    
    void Add(const InstanceKey& key);
    void Remove(const InstanceKey& key);
    bool Contains(const InstanceKey& key) const;
    size_t Size() const;
    void Clear();
    
    // Get all keys
    std::vector<InstanceKey> GetAll() const;
    
    // Find by host
    std::optional<InstanceKey> FindByHost(const std::string& host) const;
    
private:
    std::vector<InstanceKey> keys_;
};

// Replica terminology map (master -> replica terminology conversion)
class ReplicaTerminologyMap {
public:
    ReplicaTerminologyMap();
    
    // Convert master terminology to replica terminology
    std::string MasterToReplica(const std::string& term) const;
    std::string ReplicaToMaster(const std::string& term) const;
    
    // Common terminology
    static const std::string MASTER;
    static const std::string REPLICA;
    static const std::string SLAVE;  // Deprecated term
    
private:
    bool use_replica_term_;  // Use 'replica' instead of 'slave'
};

} // namespace gh_ost

// Hash function for InstanceKey (for use in unordered_map etc)
namespace std {
    template<>
    struct hash<gh_ost::InstanceKey> {
        size_t operator()(const gh_ost::InstanceKey& key) const {
            return hash<string>()(key.Host()) ^ (hash<uint16_t>()(key.Port()) << 1);
        }
    };
}

#endif // GH_OST_INSTANCE_KEY_HPP