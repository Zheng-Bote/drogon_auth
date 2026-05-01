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
            "SELECT id, loginname, email, is_active, created_at FROM users"
        );

        Json::Value ret(Json::arrayValue);
        for (const auto& row : res) {
            Json::Value user;
            user["id"] = row["id"].as<std::string>();
            user["loginname"] = row["loginname"].as<std::string>();
            user["email"] = row["email"].as<std::string>();
            user["is_active"] = row["is_active"].as<bool>();
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
            "INSERT INTO users (id, loginname, email, password_hash, is_active) VALUES ($1, $2, $3, $4, $5)",
            user_id, loginname, email, hash_result.value(), is_active
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

} // namespace drogon_auth
