/**
 * SPDX-FileComment: In-Memory Session Cache Implementation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */
#include "cache/session_cache.hpp"
#include "db/auth_repository.hpp"
#include <drogon/drogon.h>
#include <json/json.h>

namespace drogon_auth::cache {

drogon::Task<void> SessionCache::sync_from_db() {
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro("SELECT session_token, user_id, ip_address, user_agent, expires_at, data FROM sessions WHERE expires_at > CURRENT_TIMESTAMP");
        
        std::unordered_map<std::string, SessionMeta> new_cache;
        for (const auto& row : res) {
            SessionMeta meta;
            meta.user_id = row["user_id"].as<std::string>();
            meta.ip_address = row["ip_address"].as<std::string>();
            meta.user_agent = row["user_agent"].as<std::string>();
            meta.expires_at = row["expires_at"].as<std::string>();
            
            if (!row["data"].isNull()) {
                Json::Value data_json;
                Json::Reader reader;
                if (reader.parse(row["data"].as<std::string>(), data_json)) {
                    if (data_json.isMember("csrf_token")) {
                        meta.csrf_token = data_json["csrf_token"].asString();
                    }
                }
            }
            new_cache[row["session_token"].as<std::string>()] = meta;
        }

        {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            cache_ = std::move(new_cache);
        }
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to sync session cache from DB: " << e.what();
    }
}

} // namespace drogon_auth::cache
