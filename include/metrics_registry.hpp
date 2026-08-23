/**
 * SPDX-FileComment: Metrics Registry
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <vector>
#include "utils/ring_buffer.hpp"

namespace drogon_auth::metrics {

struct MetricEntry {
    uint64_t count{0};
    double sum_ms{0.0};
};

struct AuthEvent {
    std::string user_id;
    std::string event_type;
    std::string ip_address;
    long long timestamp_ms;
};

class MetricsRegistry {
public:
    static MetricsRegistry& instance() {
        static MetricsRegistry inst;
        return inst;
    }
    
    void record_latency(const std::string& path, double latency_ms) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& entry = latencies_[path];
        entry.count += 1;
        entry.sum_ms += latency_ms;
    }
    
    void record_auth_event(const std::string& user_id, const std::string& event_type, const std::string& ip_address) {
        AuthEvent event{user_id, event_type, ip_address, 
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()};
        auth_events_.push(event);
    }

    std::vector<AuthEvent> get_recent_auth_events() const {
        return auth_events_.snapshot();
    }

    std::unordered_map<std::string, MetricEntry> get_latencies() {
        std::lock_guard<std::mutex> lock(mutex_);
        return latencies_;
    }
private:
    MetricsRegistry() = default;
    
    std::mutex mutex_;
    std::unordered_map<std::string, MetricEntry> latencies_;
    drogon_auth::utils::LockFreeRingBuffer<AuthEvent, 100> auth_events_;
};

} // namespace drogon_auth::metrics
