/**
 * SPDX-FileComment: Authentication Repository
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <string>
#include <optional>
#include <vector>
#include <json/json.h>

namespace drogon_auth::db {

struct UserModel {
    std::string id;
    std::string loginname;
    std::string email;
    std::string password_hash;
    bool is_active{false};
    bool must_pwd_change{false};
};

struct UserCommunication {
    std::string channel;
    std::string address;
    bool is_active;
    bool verified;
};

struct UserProfileModel {
    std::string first_name;
    std::string last_name;
    std::string preferred_language;
    std::string locale;
    std::string timezone;
    std::string loginname;
    std::string email;
    std::string last_password_change;
    bool two_factor_enabled{false};
    std::vector<UserCommunication> communications;
};

class AuthRepository {
public:
    static drogon::Task<std::optional<UserModel>> find_user_by_login_or_email(const std::string& ident);
    static drogon::Task<std::optional<UserModel>> find_user_by_id(const std::string& user_id);
    static drogon::Task<std::optional<std::string>> find_user_by_email(const std::string& email);

    static drogon::Task<bool> create_user(const std::string& id, const std::string& loginname, const std::string& email, const std::string& password_hash);
    static drogon::Task<bool> update_password(const std::string& user_id, const std::string& new_password_hash);
    
    static drogon::Task<void> record_login_attempt(const std::string& user_id, const std::string& loginname, const std::string& ip_address, bool success);
    static drogon::Task<std::optional<std::string>> get_last_login_date(const std::string& user_id);

    static drogon::Task<bool> check_totp_enabled(const std::string& user_id);
    static drogon::Task<std::optional<std::string>> get_totp_secret(const std::string& user_id);
    static drogon::Task<bool> upsert_totp_secret(const std::string& user_id, const std::string& secret);

struct SessionModel {
    std::string user_id;
    std::string ip_address;
    std::string user_agent;
    std::string expires_at;
    std::string csrf_token;
};

    static drogon::Task<bool> create_session(const std::string& session_id, const std::string& user_id, const std::string& token, const std::string& expires_at_db_str, const std::string& ip, const std::string& user_agent, const std::string& csrf_token = "");
    static drogon::Task<std::optional<std::string>> get_user_id_by_session(const std::string& token);
    static drogon::Task<std::optional<SessionModel>> get_session(const std::string& token);
    static drogon::Task<std::optional<SessionModel>> get_last_session_for_user(const std::string& user_id);
    static drogon::Task<bool> refresh_session(const std::string& token, const std::string& new_expires_at_db_str);
    static drogon::Task<bool> delete_session(const std::string& token);

    static drogon::Task<bool> create_password_reset(const std::string& user_id, const std::string& token, const std::string& expires_at_db_str);
    static drogon::Task<std::optional<std::string>> find_user_by_reset_token(const std::string& token);
    static drogon::Task<bool> mark_reset_token_used(const std::string& token);

    static drogon::Task<std::optional<UserProfileModel>> get_user_profile(const std::string& user_id);
    static drogon::Task<bool> update_user_profile(const std::string& user_id, const Json::Value& profile_json);
    
    // RBAC functions (Sprint 5)
    static drogon::Task<std::vector<std::string>> get_user_permissions(const std::string& user_id);
    static drogon::Task<std::vector<std::string>> get_user_roles(const std::string& user_id);
};

} // namespace drogon_auth::db
