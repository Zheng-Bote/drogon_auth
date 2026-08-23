/**
 * SPDX-FileComment: Admin Repository Implementation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */
#include "db/admin_repository.hpp"
#include <drogon/drogon.h>

namespace drogon_auth::db {

drogon::Task<Json::Value> AdminRepository::get_all_users() {
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro(
            "SELECT id, loginname, email, is_active, must_pwd_change, created_at FROM users"
        );
        Json::Value ret(Json::arrayValue);
        for (const auto& row : res) {
            Json::Value user;
            user["id"] = row["id"].as<std::string>();
            user["loginname"] = row["loginname"].as<std::string>();
            user["email"] = row["email"].as<std::string>();
            user["is_active"] = row["is_active"].as<bool>();
            user["must_pwd_change"] = row["must_pwd_change"].as<bool>();
            user["created_at"] = row["created_at"].as<std::string>();
            ret.append(user);
        }

        auto roles_res = co_await db->execSqlCoro(
            "SELECT ur.user_id, r.name FROM user_roles ur JOIN roles r ON ur.role_id = r.id"
        );
        for (auto& user : ret) {
            Json::Value roles(Json::arrayValue);
            for (const auto& r_row : roles_res) {
                if (r_row["user_id"].as<std::string>() == user["id"].asString()) {
                    roles.append(r_row["name"].as<std::string>());
                }
            }
            user["roles"] = roles;
        }
        co_return ret;
    } catch (...) {
        co_return Json::Value();
    }
}

drogon::Task<std::optional<std::string>> AdminRepository::create_user(const std::string& id, const std::string& loginname, const std::string& email, const std::string& password_hash, bool is_active, bool must_pwd_change, const Json::Value& roles) {
    auto db = drogon::app().getDbClient();
    try {
        auto trans = co_await db->newTransactionCoro();
        co_await trans->execSqlCoro(
            "INSERT INTO users (id, loginname, email, password_hash, is_active, must_pwd_change) VALUES ($1, $2, $3, $4, $5, $6)",
            id, loginname, email, password_hash, is_active, must_pwd_change
        );
        co_await trans->execSqlCoro("INSERT INTO user_profiles (id, user_id) VALUES ($1, $2)", drogon::utils::getUuid(), id);

        if (roles.isArray()) {
            for (const auto& role_name : roles) {
                auto r_res = co_await trans->execSqlCoro("SELECT id FROM roles WHERE name = $1", role_name.asString());
                if (!r_res.empty()) {
                    co_await trans->execSqlCoro("INSERT INTO user_roles (user_id, role_id) VALUES ($1, $2)", id, r_res[0]["id"].as<std::string>());
                }
            }
        }
        co_await trans->execSqlCoro("COMMIT");
        co_return id;
    } catch (...) {
        co_return std::nullopt;
    }
}

drogon::Task<bool> AdminRepository::update_user(const std::string& user_id, const Json::Value& json) {
    auto db = drogon::app().getDbClient();
    try {
        auto trans = co_await db->newTransactionCoro();
        if (json.isMember("loginname")) {
            co_await trans->execSqlCoro("UPDATE users SET loginname = $1 WHERE id = $2", json["loginname"].asString(), user_id);
        }
        if (json.isMember("email")) {
            co_await trans->execSqlCoro("UPDATE users SET email = $1 WHERE id = $2", json["email"].asString(), user_id);
        }
        if (json.isMember("is_active")) {
            co_await trans->execSqlCoro("UPDATE users SET is_active = $1 WHERE id = $2", json["is_active"].asBool(), user_id);
        }
        if (json.isMember("must_pwd_change")) {
            co_await trans->execSqlCoro("UPDATE users SET must_pwd_change = $1 WHERE id = $2", json["must_pwd_change"].asBool(), user_id);
        }
        if (json.isMember("password_hash")) {
            co_await trans->execSqlCoro("UPDATE users SET password_hash = $1 WHERE id = $2", json["password_hash"].asString(), user_id);
        }

        if (json.isMember("roles") && json["roles"].isArray()) {
            co_await trans->execSqlCoro("DELETE FROM user_roles WHERE user_id = $1", user_id);
            for (const auto& role_name : json["roles"]) {
                auto r_res = co_await trans->execSqlCoro("SELECT id FROM roles WHERE name = $1", role_name.asString());
                if (!r_res.empty()) {
                    co_await trans->execSqlCoro("INSERT INTO user_roles (user_id, role_id) VALUES ($1, $2)", user_id, r_res[0]["id"].as<std::string>());
                }
            }
        }
        co_await trans->execSqlCoro("COMMIT");
        co_return true;
    } catch (...) {
        co_return false;
    }
}

drogon::Task<bool> AdminRepository::delete_user(const std::string& user_id) {
    auto db = drogon::app().getDbClient();
    try {
        co_await db->execSqlCoro("UPDATE users SET is_active = false WHERE id = $1", user_id);
        co_return true;
    } catch (...) {
        co_return false;
    }
}

drogon::Task<Json::Value> AdminRepository::get_all_roles() {
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro("SELECT id, name, description FROM roles");
        Json::Value ret(Json::arrayValue);
        for (const auto& row : res) {
            Json::Value role;
            role["id"] = row["id"].as<std::string>();
            role["name"] = row["name"].as<std::string>();
            role["description"] = row["description"].as<std::string>();
            ret.append(role);
        }
        co_return ret;
    } catch (...) {
        co_return Json::Value();
    }
}

drogon::Task<std::optional<std::string>> AdminRepository::create_role(const std::string& name, const std::string& description) {
    auto db = drogon::app().getDbClient();
    try {
        std::string role_id = drogon::utils::getUuid();
        co_await db->execSqlCoro("INSERT INTO roles (id, name, description) VALUES ($1, $2, $3)", role_id, name, description);
        co_return role_id;
    } catch (...) {
        co_return std::nullopt;
    }
}

drogon::Task<bool> AdminRepository::update_role(const std::string& role_id, const std::string& name, const std::string& description) {
    auto db = drogon::app().getDbClient();
    try {
        if (!name.empty()) {
            co_await db->execSqlCoro("UPDATE roles SET name = $1 WHERE id = $2", name, role_id);
        }
        if (!description.empty()) {
            co_await db->execSqlCoro("UPDATE roles SET description = $1 WHERE id = $2", description, role_id);
        }
        co_return true;
    } catch (...) {
        co_return false;
    }
}

drogon::Task<bool> AdminRepository::delete_role(const std::string& role_id) {
    auto db = drogon::app().getDbClient();
    try {
        co_await db->execSqlCoro("DELETE FROM roles WHERE id = $1", role_id);
        co_return true;
    } catch (...) {
        co_return false;
    }
}

drogon::Task<Json::Value> AdminRepository::get_audit_summary() {
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro(
            "SELECT action, count(*) as count FROM audit_logs "
            "WHERE created_at > CURRENT_TIMESTAMP - INTERVAL '7 days' "
            "GROUP BY action ORDER BY count DESC"
        );

        Json::Value ret(Json::arrayValue);
        for (const auto& row : res) {
            Json::Value item;
            item["action"] = row["action"].as<std::string>();
            item["count"] = row["count"].as<int64_t>();
            ret.append(item);
        }
        co_return ret;
    } catch (...) {
        co_return Json::Value();
    }
}

} // namespace drogon_auth::db
