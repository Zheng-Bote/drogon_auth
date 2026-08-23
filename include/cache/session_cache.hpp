/**
 * SPDX-FileComment: In-Memory Session Cache
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <drogon/utils/coroutine.h>

namespace drogon_auth::cache {

struct SessionMeta {
    std::string user_id;
    std::string ip_address;
    std::string user_agent;
    std::string expires_at;
    std::string csrf_token;
};

class SessionCache {
public:
    static SessionCache& instance() {
        static SessionCache inst;
        return inst;
    }

    std::optional<SessionMeta> get_session(const std::string& token) {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = cache_.find(token);
        if (it != cache_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    void put_session(const std::string& token, const SessionMeta& meta) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        cache_[token] = meta;
    }

    void remove_session(const std::string& token) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        cache_.erase(token);
    }

    drogon::Task<void> sync_from_db();

private:
    SessionCache() = default;
    std::shared_mutex mutex_;
    std::unordered_map<std::string, SessionMeta> cache_;
};

} // namespace drogon_auth::cache
