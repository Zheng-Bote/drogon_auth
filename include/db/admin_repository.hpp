/**
 * SPDX-FileComment: Admin Repository
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <string>
#include <vector>
#include <json/json.h>

namespace drogon_auth::db {

class AdminRepository {
public:
    static drogon::Task<Json::Value> get_all_users();
    static drogon::Task<std::optional<std::string>> create_user(const std::string& id, const std::string& loginname, const std::string& email, const std::string& password_hash, bool is_active, bool must_pwd_change, const Json::Value& roles);
    static drogon::Task<bool> update_user(const std::string& user_id, const Json::Value& json);
    static drogon::Task<bool> delete_user(const std::string& user_id);
    
    static drogon::Task<Json::Value> get_all_roles();
    static drogon::Task<std::optional<std::string>> create_role(const std::string& name, const std::string& description);
    static drogon::Task<bool> update_role(const std::string& role_id, const std::string& name, const std::string& description);
    static drogon::Task<bool> delete_role(const std::string& role_id);

    static drogon::Task<Json::Value> get_audit_summary();
};

} // namespace drogon_auth::db
