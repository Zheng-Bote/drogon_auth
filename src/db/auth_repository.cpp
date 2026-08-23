/**
 * SPDX-FileComment: Authentication Repository Implementation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */
#include "db/auth_repository.hpp"
#include <drogon/drogon.h>
#include <print>

namespace drogon_auth::db {

drogon::Task<std::optional<UserModel>> AuthRepository::find_user_by_login_or_email(const std::string& ident) {
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro("SELECT id, loginname, email, password_hash, is_active, must_pwd_change FROM users WHERE loginname = $1 OR email = $1", ident);
        if (res.empty()) co_return std::nullopt;
        UserModel m;
        m.id = res[0]["id"].as<std::string>();
        m.loginname = res[0]["loginname"].as<std::string>();
        m.email = res[0]["email"].as<std::string>();
        m.password_hash = res[0]["password_hash"].as<std::string>();
        m.is_active = res[0]["is_active"].as<bool>();
        m.must_pwd_change = res[0]["must_pwd_change"].as<bool>();
        co_return m;
    } catch (const drogon::orm::DrogonDbException &e) {
        std::println(stderr, "DB Error in find_user_by_login_or_email: {}", e.base().what());
        co_return std::nullopt;
    }
}

drogon::Task<std::optional<UserModel>> AuthRepository::find_user_by_id(const std::string& user_id) {
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro("SELECT id, loginname, email, password_hash, is_active, must_pwd_change FROM users WHERE id = $1", user_id);
        if (res.empty()) co_return std::nullopt;
        UserModel m;
        m.id = res[0]["id"].as<std::string>();
        m.loginname = res[0]["loginname"].as<std::string>();
        m.email = res[0]["email"].as<std::string>();
        m.password_hash = res[0]["password_hash"].as<std::string>();
        m.is_active = res[0]["is_active"].as<bool>();
        m.must_pwd_change = res[0]["must_pwd_change"].as<bool>();
        co_return m;
    } catch (...) {
        co_return std::nullopt;
    }
}

drogon::Task<std::optional<std::string>> AuthRepository::find_user_by_email(const std::string& email) {
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro("SELECT id FROM users WHERE email = $1", email);
        if (res.empty()) co_return std::nullopt;
        co_return res[0]["id"].as<std::string>();
    } catch (...) {
        co_return std::nullopt;
    }
}

drogon::Task<bool> AuthRepository::create_user(const std::string& id, const std::string& loginname, const std::string& email, const std::string& password_hash) {
    auto db = drogon::app().getDbClient();
    try {
        auto trans = co_await db->newTransactionCoro();
        co_await trans->execSqlCoro("INSERT INTO users (id, loginname, email, password_hash) VALUES ($1, $2, $3, $4)", id, loginname, email, password_hash);
        co_await trans->execSqlCoro("INSERT INTO user_profiles (id, user_id) VALUES ($1, $2)", drogon::utils::getUuid(), id);
        co_await trans->execSqlCoro("INSERT INTO user_communications (id, user_id, channel, address) VALUES ($1, $2, $3, $4)", drogon::utils::getUuid(), id, "email", email);
        co_await trans->execSqlCoro("COMMIT");
        co_return true;
    } catch (const drogon::orm::DrogonDbException &e) {
        std::println(stderr, "DB Error in create_user: {}", e.base().what());
        co_return false;
    }
}

drogon::Task<bool> AuthRepository::update_password(const std::string& user_id, const std::string& new_password_hash) {
    auto db = drogon::app().getDbClient();
    try {
        co_await db->execSqlCoro("UPDATE users SET password_hash = $1, must_pwd_change = false WHERE id = $2", new_password_hash, user_id);
        co_return true;
    } catch (...) {
        co_return false;
    }
}

drogon::Task<void> AuthRepository::record_login_attempt(const std::string& user_id, const std::string& loginname, const std::string& ip_address, bool success) {
    auto db = drogon::app().getDbClient();
    try {
        co_await db->execSqlCoro("INSERT INTO login_attempts (id, user_id, loginname, ip_address, success) VALUES ($1, $2, $3, CAST($4 AS INET), $5)",
            drogon::utils::getUuid(), user_id, loginname, ip_address, success);
    } catch (...) {}
}

drogon::Task<std::optional<std::string>> AuthRepository::get_last_login_date(const std::string& user_id) {
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro("SELECT created_at FROM login_attempts WHERE user_id = $1 AND success = true ORDER BY created_at DESC LIMIT 1 OFFSET 1", user_id);
        if (res.empty()) co_return std::nullopt;
        co_return res[0]["created_at"].as<std::string>();
    } catch (...) {
        co_return std::nullopt;
    }
}

drogon::Task<bool> AuthRepository::check_totp_enabled(const std::string& user_id) {
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro("SELECT 1 FROM totp_secrets WHERE user_id = $1", user_id);
        co_return !res.empty();
    } catch (...) {
        co_return false;
    }
}

drogon::Task<std::optional<std::string>> AuthRepository::get_totp_secret(const std::string& user_id) {
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro("SELECT secret FROM totp_secrets WHERE user_id = $1", user_id);
        if (res.empty()) co_return std::nullopt;
        co_return res[0]["secret"].as<std::string>();
    } catch (...) {
        co_return std::nullopt;
    }
}

drogon::Task<bool> AuthRepository::upsert_totp_secret(const std::string& user_id, const std::string& secret) {
    auto db = drogon::app().getDbClient();
    try {
        auto trans = co_await db->newTransactionCoro();
        auto existing = co_await trans->execSqlCoro("SELECT id FROM totp_secrets WHERE user_id = $1", user_id);
        if (existing.empty()) {
            co_await trans->execSqlCoro("INSERT INTO totp_secrets (id, user_id, secret, issuer) VALUES ($1, $2, $3, $4)", drogon::utils::getUuid(), user_id, secret, "Drogon Auth");
        } else {
            co_await trans->execSqlCoro("UPDATE totp_secrets SET secret = $1 WHERE user_id = $2", secret, user_id);
        }
        co_await trans->execSqlCoro("COMMIT");
        co_return true;
    } catch (...) {
        co_return false;
    }
}

drogon::Task<bool> AuthRepository::create_session(const std::string& session_id, const std::string& user_id, const std::string& token, const std::string& expires_at_db_str, const std::string& ip, const std::string& user_agent, const std::string& csrf_token) {
    auto db = drogon::app().getDbClient();
    try {
        Json::Value data;
        if (!csrf_token.empty()) data["csrf_token"] = csrf_token;
        std::string data_str = data.empty() ? "{}" : data.toStyledString();
        
        auto res = co_await db->execSqlCoro("INSERT INTO sessions (id, user_id, session_token, expires_at, ip_address, user_agent, data) VALUES ($1, $2, $3, $4, CAST($5 AS INET), $6, CAST($7 AS JSONB))", 
                                            session_id, user_id, token, expires_at_db_str, ip, user_agent, data_str);
        co_return res.affectedRows() > 0;
    } catch (...) {
        co_return false;
    }
}

drogon::Task<std::optional<std::string>> AuthRepository::get_user_id_by_session(const std::string& token) {
    auto db = drogon::app().getDbClient();
    auto res = co_await db->execSqlCoro("SELECT user_id FROM sessions WHERE session_token = $1 AND expires_at > CURRENT_TIMESTAMP", token);
    if (res.empty()) co_return std::nullopt;
    co_return res[0]["user_id"].as<std::string>();
}

drogon::Task<std::optional<AuthRepository::SessionModel>> AuthRepository::get_session(const std::string& token) {
    auto db = drogon::app().getDbClient();
    auto res = co_await db->execSqlCoro("SELECT user_id, ip_address, user_agent, expires_at, data FROM sessions WHERE session_token = $1 AND expires_at > CURRENT_TIMESTAMP", token);
    if (res.empty()) co_return std::nullopt;
    
    SessionModel model;
    model.user_id = res[0]["user_id"].as<std::string>();
    model.ip_address = res[0]["ip_address"].as<std::string>();
    model.user_agent = res[0]["user_agent"].as<std::string>();
    model.expires_at = res[0]["expires_at"].as<std::string>();
    
    if (!res[0]["data"].isNull()) {
        try {
            Json::Value data_json;
            Json::Reader reader;
            if (reader.parse(res[0]["data"].as<std::string>(), data_json)) {
                if (data_json.isMember("csrf_token")) {
                    model.csrf_token = data_json["csrf_token"].asString();
                }
            }
        } catch(...) {}
    }
    
    co_return model;
}

drogon::Task<std::optional<AuthRepository::SessionModel>> AuthRepository::get_last_session_for_user(const std::string& user_id) {
    auto db = drogon::app().getDbClient();
    // Use last_accessed_at if available, else created_at or expires_at. Wait, we have last_accessed_at ? 
    // In refresh_session we do "last_accessed_at = CURRENT_TIMESTAMP". So yes, we have it, or we can just sort by id or created_at if it exists.
    // Let's sort by expires_at DESC since we don't know the exact schema for creation time.
    auto res = co_await db->execSqlCoro("SELECT user_id, ip_address, user_agent, expires_at, data FROM sessions WHERE user_id = $1 ORDER BY expires_at DESC LIMIT 1", user_id);
    if (res.empty()) co_return std::nullopt;
    
    SessionModel model;
    model.user_id = res[0]["user_id"].as<std::string>();
    model.ip_address = res[0]["ip_address"].as<std::string>();
    model.user_agent = res[0]["user_agent"].as<std::string>();
    model.expires_at = res[0]["expires_at"].as<std::string>();
    co_return model;
}

drogon::Task<bool> AuthRepository::refresh_session(const std::string& token, const std::string& new_expires_at_db_str) {
    auto db = drogon::app().getDbClient();
    auto res = co_await db->execSqlCoro("UPDATE sessions SET expires_at = $1, last_accessed_at = CURRENT_TIMESTAMP WHERE session_token = $2", new_expires_at_db_str, token);
    co_return res.affectedRows() > 0;
}

drogon::Task<bool> AuthRepository::delete_session(const std::string& token) {
    auto db = drogon::app().getDbClient();
    try {
        co_await db->execSqlCoro("DELETE FROM sessions WHERE session_token = $1", token);
        co_return true;
    } catch (...) {
        co_return false;
    }
}

drogon::Task<bool> AuthRepository::create_password_reset(const std::string& user_id, const std::string& token, const std::string& expires_at_db_str) {
    auto db = drogon::app().getDbClient();
    try {
        co_await db->execSqlCoro("INSERT INTO password_resets (id, user_id, token, expires_at) VALUES ($1, $2, $3, $4)",
            drogon::utils::getUuid(), user_id, token, expires_at_db_str);
        co_return true;
    } catch (...) {
        co_return false;
    }
}

drogon::Task<std::optional<std::string>> AuthRepository::find_user_by_reset_token(const std::string& token) {
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro("SELECT user_id FROM password_resets WHERE token = $1 AND expires_at > CURRENT_TIMESTAMP AND used = 0", token);
        if (res.empty()) co_return std::nullopt;
        co_return res[0]["user_id"].as<std::string>();
    } catch (...) {
        co_return std::nullopt;
    }
}

drogon::Task<bool> AuthRepository::mark_reset_token_used(const std::string& token) {
    auto db = drogon::app().getDbClient();
    try {
        co_await db->execSqlCoro("UPDATE password_resets SET used = 1 WHERE token = $1", token);
        co_return true;
    } catch (...) {
        co_return false;
    }
}

drogon::Task<std::optional<UserProfileModel>> AuthRepository::get_user_profile(const std::string& user_id) {
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro(
            "SELECT p.first_name, p.last_name, p.preferred_language, p.locale, p.timezone, u.loginname, u.email, u.updated_at as last_pwd_change "
            "FROM user_profiles p JOIN users u ON p.user_id = u.id WHERE p.user_id = $1", user_id
        );
        if (res.empty()) co_return std::nullopt;

        UserProfileModel p;
        p.first_name = res[0]["first_name"].as<std::string>();
        p.last_name = res[0]["last_name"].as<std::string>();
        p.preferred_language = res[0]["preferred_language"].as<std::string>();
        p.locale = res[0]["locale"].as<std::string>();
        p.timezone = res[0]["timezone"].as<std::string>();
        p.loginname = res[0]["loginname"].as<std::string>();
        p.email = res[0]["email"].as<std::string>();
        p.last_password_change = res[0]["last_pwd_change"].as<std::string>();

        p.two_factor_enabled = co_await check_totp_enabled(user_id);

        auto comm_res = co_await db->execSqlCoro("SELECT channel, address, is_active, verified FROM user_communications WHERE user_id = $1", user_id);
        for (const auto& row : comm_res) {
            UserCommunication c;
            c.channel = row["channel"].as<std::string>();
            c.address = row["address"].as<std::string>();
            c.is_active = row["is_active"].as<bool>();
            c.verified = row["verified"].as<bool>();
            p.communications.push_back(c);
        }

        co_return p;
    } catch (...) {
        co_return std::nullopt;
    }
}

drogon::Task<bool> AuthRepository::update_user_profile(const std::string& user_id, const Json::Value& json) {
    auto db = drogon::app().getDbClient();
    try {
        auto trans = co_await db->newTransactionCoro();
        co_await trans->execSqlCoro("INSERT INTO user_profiles (id, user_id) VALUES ($1, $2) ON CONFLICT (user_id) DO NOTHING", drogon::utils::getUuid(), user_id);
        
        if (json.isMember("first_name")) co_await trans->execSqlCoro("UPDATE user_profiles SET first_name = $1 WHERE user_id = $2", json["first_name"].asString(), user_id);
        if (json.isMember("last_name")) co_await trans->execSqlCoro("UPDATE user_profiles SET last_name = $1 WHERE user_id = $2", json["last_name"].asString(), user_id);
        if (json.isMember("preferred_language")) co_await trans->execSqlCoro("UPDATE user_profiles SET preferred_language = $1 WHERE user_id = $2", json["preferred_language"].asString(), user_id);
        if (json.isMember("timezone")) co_await trans->execSqlCoro("UPDATE user_profiles SET timezone = $1 WHERE user_id = $2", json["timezone"].asString(), user_id);

        if (json.isMember("communications") && json["communications"].isArray()) {
            co_await trans->execSqlCoro("DELETE FROM user_communications WHERE user_id = $1", user_id);
            for (const auto& c : json["communications"]) {
                co_await trans->execSqlCoro(
                    "INSERT INTO user_communications (id, user_id, channel, address, is_active) VALUES ($1, $2, CAST($3 AS communication_channel), $4, $5)",
                    drogon::utils::getUuid(), user_id, c["channel"].asString(), c["address"].asString(), c.get("is_active", true).asBool()
                );
            }
        }
        co_await trans->execSqlCoro("COMMIT");
        co_return true;
    } catch (...) {
        co_return false;
    }
}

drogon::Task<std::vector<std::string>> AuthRepository::get_user_permissions(const std::string& user_id) {
    auto db = drogon::app().getDbClient();
    std::vector<std::string> perms;
    try {
        auto res = co_await db->execSqlCoro(
            "SELECT p.name FROM permissions p "
            "JOIN role_permissions rp ON p.id = rp.permission_id "
            "JOIN user_roles ur ON rp.role_id = ur.role_id "
            "WHERE ur.user_id = $1", user_id
        );
        for (const auto& row : res) {
            perms.push_back(row["name"].as<std::string>());
        }
        co_return perms;
    } catch (...) {
        co_return perms;
    }
}

drogon::Task<std::vector<std::string>> AuthRepository::get_user_roles(const std::string& user_id) {
    auto db = drogon::app().getDbClient();
    std::vector<std::string> roles;
    try {
        auto res = co_await db->execSqlCoro(
            "SELECT r.name FROM roles r "
            "JOIN user_roles ur ON r.id = ur.role_id "
            "WHERE ur.user_id = $1", user_id
        );
        for (const auto& row : res) {
            roles.push_back(row["name"].as<std::string>());
        }
        co_return roles;
    } catch (...) {
        co_return roles;
    }
}

} // namespace drogon_auth::db
