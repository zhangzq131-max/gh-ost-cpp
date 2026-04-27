/*
 * gh-ost-cpp - GitHub's Online Schema Transmogrifier for MySQL (C++ Implementation)
 * Copyright 2026
 * Licensed under MIT License
 */

#include "mysql/instance_key.hpp"
#include "utils/string_utils.hpp"

namespace gh_ost {

// InstanceKey implementation
InstanceKey::InstanceKey() : host_(""), port_(0) {}

InstanceKey::InstanceKey(const std::string& host, uint16_t port)
    : host_(host), port_(port) {}

InstanceKey::InstanceKey(const std::string& uri) {
    auto parsed = Parse(uri);
    if (parsed) {
        host_ = parsed->host_;
        port_ = parsed->port_;
    } else {
        host_ = "";
        port_ = 0;
    }
}

std::string InstanceKey::ToString() const {
    if (IsValid()) {
        return host_ + ":" + std::to_string(port_);
    }
    return "";
}

std::string InstanceKey::ToURI() const {
    return ToString();
}

std::string InstanceKey::ToDisplayString() const {
    return ToString();
}

bool InstanceKey::IsSameAs(const InstanceKey& other) const {
    return host_ == other.host_ && port_ == other.port_;
}

bool InstanceKey::IsValid() const {
    return !host_.empty() && port_ > 0;
}

std::optional<InstanceKey> InstanceKey::Parse(const std::string& str) {
    auto parts = StringUtils::Split(str, ':');
    if (parts.size() != 2) {
        return std::nullopt;
    }
    
    std::string host = StringUtils::Trim(parts[0]);
    auto port_opt = StringUtils::ParseUInt16(parts[1]);
    
    if (!port_opt) {
        return std::nullopt;
    }
    
    return InstanceKey(host, *port_opt);
}

bool InstanceKey::operator==(const InstanceKey& other) const {
    return IsSameAs(other);
}

bool InstanceKey::operator!=(const InstanceKey& other) const {
    return !IsSameAs(other);
}

bool InstanceKey::operator<(const InstanceKey& other) const {
    if (host_ != other.host_) return host_ < other.host_;
    return port_ < other.port_;
}

// InstanceKeyMap implementation
InstanceKeyMap::InstanceKeyMap() {}

void InstanceKeyMap::Add(const InstanceKey& key) {
    if (!Contains(key)) {
        keys_.push_back(key);
    }
}

void InstanceKeyMap::Remove(const InstanceKey& key) {
    keys_.erase(
        std::remove(keys_.begin(), keys_.end(), key),
        keys_.end()
    );
}

bool InstanceKeyMap::Contains(const InstanceKey& key) const {
    return std::find(keys_.begin(), keys_.end(), key) != keys_.end();
}

size_t InstanceKeyMap::Size() const {
    return keys_.size();
}

void InstanceKeyMap::Clear() {
    keys_.clear();
}

std::vector<InstanceKey> InstanceKeyMap::GetAll() const {
    return keys_;
}

std::optional<InstanceKey> InstanceKeyMap::FindByHost(const std::string& host) const {
    for (const auto& key : keys_) {
        if (key.Host() == host) {
            return key;
        }
    }
    return std::nullopt;
}

// ReplicaTerminologyMap implementation
const std::string ReplicaTerminologyMap::MASTER = "master";
const std::string ReplicaTerminologyMap::REPLICA = "replica";
const std::string ReplicaTerminologyMap::SLAVE = "slave";

ReplicaTerminologyMap::ReplicaTerminologyMap() : use_replica_term_(true) {}

std::string ReplicaTerminologyMap::MasterToReplica(const std::string& term) const {
    if (use_replica_term_) {
        if (term == SLAVE) return REPLICA;
    }
    return term;
}

std::string ReplicaTerminologyMap::ReplicaToMaster(const std::string& term) const {
    if (!use_replica_term_) {
        if (term == REPLICA) return SLAVE;
    }
    return term;
}

} // namespace gh_ost