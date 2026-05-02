/**
 * SPDX-FileComment: Admin Controller Implementation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */
#include "admin_ctrl.hpp"
#include "auth_srv.hpp"
#include <drogon/utils/Utilities.h>
#include <print>

namespace drogon_auth {

drogon::Task<bool> AdminCtrl::is_admin(const drogon::HttpRequestPtr& req) {
    auto session = req->session();
    if (!session || !session->find("user_id")) co_return false;
    
    std::string user_id = session->get<std::string>("user_id");
    auto db = drogon::app().getDbClient();
    
    try {
        auto res = co_await db->execSqlCoro(
            "SELECT 1 FROM user_roles ur JOIN roles r ON ur.role_id = r.id "
            "WHERE ur.user_id = $1 AND r.name = 'admin'", user_id);
        co_return !res.empty();
    } catch (...) {
        co_return false;
    }
}

drogon::Task<drogon::HttpResponsePtr> AdminCtrl::list_users(drogon::HttpRequestPtr req) {
    if (!co_await is_admin(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        co_return resp;
    }

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

        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;
    } catch (const std::exception& e) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AdminCtrl::create_user(drogon::HttpRequestPtr req) {
    if (!co_await is_admin(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        co_return resp;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("loginname") || !json->isMember("email") || !json->isMember("password")) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        co_return resp;
    }

    std::string loginname = (*json)["loginname"].asString();
    std::string email = (*json)["email"].asString();
    std::string password = (*json)["password"].asString();
    bool is_active = json->isMember("is_active") ? (*json)["is_active"].asBool() : true;
    bool must_pwd_change = json->isMember("must_pwd_change") ? (*json)["must_pwd_change"].asBool() : false;

    auto hash_result = AuthSrv::hash_password(password);
    if (!hash_result) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }

    auto db = drogon::app().getDbClient();
    try {
        auto trans = co_await db->newTransactionCoro();
        std::string user_id = drogon::utils::getUuid();
        
        co_await trans->execSqlCoro(
            "INSERT INTO users (id, loginname, email, password_hash, is_active, must_pwd_change) VALUES ($1, $2, $3, $4, $5, $6)",
            user_id, loginname, email, hash_result.value(), is_active, must_pwd_change
        );
        
        co_await trans->execSqlCoro("INSERT INTO user_profiles (id, user_id) VALUES ($1, $2)", drogon::utils::getUuid(), user_id);

        if (json->isMember("roles") && (*json)["roles"].isArray()) {
            for (const auto& role_name : (*json)["roles"]) {
                auto r_res = co_await trans->execSqlCoro("SELECT id FROM roles WHERE name = $1", role_name.asString());
                if (!r_res.empty()) {
                    co_await trans->execSqlCoro("INSERT INTO user_roles (user_id, role_id) VALUES ($1, $2)", user_id, r_res[0]["id"].as<std::string>());
                }
            }
        }
        co_await trans->execSqlCoro("COMMIT");

        Json::Value ret;
        ret["status"] = "success";
        ret["id"] = user_id;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;
    } catch (const drogon::orm::DrogonDbException& e) {
        Json::Value ret;
        ret["status"] = "error";
        std::string err_msg = e.base().what();
        if (err_msg.find("unique constraint") != std::string::npos || err_msg.find("already exists") != std::string::npos) {
            ret["message"] = "Login name or email already exists";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(drogon::k409Conflict);
            co_return resp;
        }
        ret["message"] = "Database error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    } catch (...) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AdminCtrl::update_user(drogon::HttpRequestPtr req, std::string user_id) {
    if (!co_await is_admin(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        co_return resp;
    }

    auto json = req->getJsonObject();
    if (!json) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        co_return resp;
    }

    auto db = drogon::app().getDbClient();
    try {
        auto trans = co_await db->newTransactionCoro();
        
        if (json->isMember("loginname")) {
            co_await trans->execSqlCoro("UPDATE users SET loginname = $1 WHERE id = $2", (*json)["loginname"].asString(), user_id);
        }
        if (json->isMember("email")) {
            co_await trans->execSqlCoro("UPDATE users SET email = $1 WHERE id = $2", (*json)["email"].asString(), user_id);
        }
        if (json->isMember("is_active")) {
            co_await trans->execSqlCoro("UPDATE users SET is_active = $1 WHERE id = $2", (*json)["is_active"].asBool(), user_id);
        }
        if (json->isMember("must_pwd_change")) {
            co_await trans->execSqlCoro("UPDATE users SET must_pwd_change = $1 WHERE id = $2", (*json)["must_pwd_change"].asBool(), user_id);
        }
        if (json->isMember("password") && !(*json)["password"].asString().empty()) {
            auto hash = AuthSrv::hash_password((*json)["password"].asString());
            if (hash) {
                co_await trans->execSqlCoro("UPDATE users SET password_hash = $1 WHERE id = $2", hash.value(), user_id);
            }
        }

        if (json->isMember("roles") && (*json)["roles"].isArray()) {
            co_await trans->execSqlCoro("DELETE FROM user_roles WHERE user_id = $1", user_id);
            for (const auto& role_name : (*json)["roles"]) {
                auto r_res = co_await trans->execSqlCoro("SELECT id FROM roles WHERE name = $1", role_name.asString());
                if (!r_res.empty()) {
                    co_await trans->execSqlCoro("INSERT INTO user_roles (user_id, role_id) VALUES ($1, $2)", user_id, r_res[0]["id"].as<std::string>());
                }
            }
        }
        co_await trans->execSqlCoro("COMMIT");

        Json::Value ret;
        ret["status"] = "success";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;
    } catch (...) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AdminCtrl::delete_user(drogon::HttpRequestPtr req, std::string user_id) {
    if (!co_await is_admin(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        co_return resp;
    }

    auto db = drogon::app().getDbClient();
    try {
        co_await db->execSqlCoro("UPDATE users SET is_active = false WHERE id = $1", user_id);
        Json::Value ret;
        ret["status"] = "success";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;
    } catch (...) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AdminCtrl::list_roles(drogon::HttpRequestPtr req) {
    if (!co_await is_admin(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        co_return resp;
    }

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
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;
    } catch (...) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AdminCtrl::create_role(drogon::HttpRequestPtr req) {
    if (!co_await is_admin(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        co_return resp;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("name")) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        co_return resp;
    }

    std::string name = (*json)["name"].asString();
    std::string description = json->isMember("description") ? (*json)["description"].asString() : "";

    auto db = drogon::app().getDbClient();
    try {
        std::string role_id = drogon::utils::getUuid();
        co_await db->execSqlCoro(
            "INSERT INTO roles (id, name, description) VALUES ($1, $2, $3)",
            role_id, name, description
        );

        Json::Value ret;
        ret["status"] = "success";
        ret["id"] = role_id;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;
    } catch (const drogon::orm::DrogonDbException& e) {
        Json::Value ret;
        ret["status"] = "error";
        std::string err_msg = e.base().what();
        if (err_msg.find("unique constraint") != std::string::npos || err_msg.find("already exists") != std::string::npos) {
            ret["message"] = "Role name already exists";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(drogon::k409Conflict);
            co_return resp;
        }
        ret["message"] = "Database error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    } catch (...) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AdminCtrl::update_role(drogon::HttpRequestPtr req, std::string role_id) {
    if (!co_await is_admin(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        co_return resp;
    }

    auto json = req->getJsonObject();
    if (!json) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        co_return resp;
    }

    auto db = drogon::app().getDbClient();
    try {
        if (json->isMember("name")) {
            co_await db->execSqlCoro("UPDATE roles SET name = $1 WHERE id = $2", (*json)["name"].asString(), role_id);
        }
        if (json->isMember("description")) {
            co_await db->execSqlCoro("UPDATE roles SET description = $1 WHERE id = $2", (*json)["description"].asString(), role_id);
        }

        Json::Value ret;
        ret["status"] = "success";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;
    } catch (...) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AdminCtrl::delete_role(drogon::HttpRequestPtr req, std::string role_id) {
    if (!co_await is_admin(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        co_return resp;
    }

    auto db = drogon::app().getDbClient();
    try {
        // Note: In a real system, you might want to check if users are still assigned to this role
        // or handle it via cascading deletes in the DB schema.
        co_await db->execSqlCoro("DELETE FROM roles WHERE id = $1", role_id);
        
        Json::Value ret;
        ret["status"] = "success";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;
    } catch (...) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AdminCtrl::get_audit_summary(drogon::HttpRequestPtr req) {
    if (!co_await is_admin(req)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        co_return resp;
    }

    auto db = drogon::app().getDbClient();
    try {
        // Last 7 days summary
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

        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;
    } catch (const std::exception& e) {
        std::println(stderr, "Error in get_audit_summary: {}", e.what());
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
}

} // namespace drogon_auth
