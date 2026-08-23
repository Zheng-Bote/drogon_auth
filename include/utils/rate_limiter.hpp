/**
 * SPDX-FileComment: Simple Thread-Safe IP Rate Limiter
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
#include <algorithm>

namespace drogon_auth::utils {

class RateLimiter {
public:
    static RateLimiter& instance() {
        static RateLimiter inst;
        return inst;
    }

    // Returns true if allowed, false if limit exceeded
    bool is_allowed(const std::string& ip, int max_requests, int window_seconds) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        
        auto& record = requests_[ip];
        
        // Clean up old requests outside the time window
        record.erase(std::remove_if(record.begin(), record.end(),
            [now, window_seconds](const auto& time) {
                return std::chrono::duration_cast<std::chrono::seconds>(now - time).count() > window_seconds;
            }), record.end());
            
        if (record.size() >= static_cast<size_t>(max_requests)) {
            return false;
        }
        
        record.push_back(now);
        return true;
    }

private:
    RateLimiter() = default;
    std::mutex mutex_;
    std::unordered_map<std::string, std::vector<std::chrono::steady_clock::time_point>> requests_;
};

} // namespace drogon_auth::utils
