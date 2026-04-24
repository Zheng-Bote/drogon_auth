/**
 * SPDX-FileComment: Authentication Controller Implementation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */
#include "auth_ctrl.hpp"
#include "auth_srv.hpp"
#include <drogon/utils/Utilities.h>
#include <print>

namespace drogon_auth {

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::register_user(drogon::HttpRequestPtr req) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("loginname") || !json->isMember("email") || !json->isMember("password")) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        co_return resp;
    }

    std::string loginname = (*json)["loginname"].asString();
    std::string email = (*json)["email"].asString();
    std::string password = (*json)["password"].asString();

    auto hash_result = AuthSrv::hash_password(password);
    if (!hash_result) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }

    auto db = drogon::app().getDbClient();
    try {
        std::string user_id = drogon::utils::getUuid(); // generates 32 hex chars

        auto trans = co_await db->newTransactionCoro();
        co_await trans->execSqlCoro(
            "INSERT INTO users (id, loginname, email, password_hash) VALUES ($1, $2, $3, $4)",
            user_id, loginname, email, hash_result.value()
        );
        co_await trans->execSqlCoro("INSERT INTO user_profiles (id, user_id) VALUES ($1, $2)", drogon::utils::getUuid(), user_id);
        co_await trans->execSqlCoro("INSERT INTO user_communications (id, user_id, channel, address) VALUES ($1, $2, $3, $4)", 
            drogon::utils::getUuid(), user_id, "email", email);
        // Note: For SQLite we generated id for all tables to satisfy PRIMARY KEY if no auto-increment

        Json::Value ret;
        ret["status"] = "success";
        ret["message"] = "User registered successfully";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;

    } catch (const drogon::orm::DrogonDbException &e) {
        std::println(stderr, "DB Error in register_user: {}", e.base().what());
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::login(drogon::HttpRequestPtr req) {
    auto json = req->getJsonObject();
    if (!json || (!json->isMember("loginname") && !json->isMember("email")) || !json->isMember("password")) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        co_return resp;
    }

    std::string ident = json->isMember("loginname") ? (*json)["loginname"].asString() : (*json)["email"].asString();
    std::string password = (*json)["password"].asString();
    std::string ip_address = req->peerAddr().toIp();
    std::string user_agent = req->getHeader("User-Agent");

    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro("SELECT id, loginname, password_hash, is_active FROM users WHERE loginname = $1 OR email = $1", ident);
        
        if (res.empty() || !res[0]["is_active"].as<bool>()) {
            // Log failed attempt
            if (!res.empty()) {
                co_await db->execSqlCoro("INSERT INTO login_attempts (id, user_id, loginname, ip_address, success) VALUES ($1, $2, $3, $4, $5)",
                    drogon::utils::getUuid(), res[0]["id"].as<std::string>(), ident, ip_address, 0);
            }
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            co_return resp;
        }

        std::string user_id = res[0]["id"].as<std::string>();
        std::string hash = res[0]["password_hash"].as<std::string>();

        if (!AuthSrv::verify_password(password, hash)) {
            co_await db->execSqlCoro("INSERT INTO login_attempts (id, user_id, loginname, ip_address, success) VALUES ($1, $2, $3, $4, $5)",
                drogon::utils::getUuid(), user_id, ident, ip_address, 0);
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            co_return resp;
        }

        // Success
        co_await db->execSqlCoro("INSERT INTO login_attempts (id, user_id, loginname, ip_address, success) VALUES ($1, $2, $3, $4, $5)",
            drogon::utils::getUuid(), user_id, ident, ip_address, 1);
        co_await db->execSqlCoro("INSERT INTO audit_logs (id, user_id, action, ip_address) VALUES ($1, $2, $3, $4)",
            drogon::utils::getUuid(), user_id, "login", ip_address);

        std::string token = AuthSrv::generate_session_token();
        // Set expiry to 24h
        auto expires_at = trantor::Date::date().after(24 * 3600);
        std::string expires_str = expires_at.toDbStringLocal(); // e.g., format for DB

        co_await db->execSqlCoro("INSERT INTO sessions (id, user_id, session_token, expires_at, ip_address, user_agent) VALUES ($1, $2, $3, $4, $5, $6)",
            drogon::utils::getUuid(), user_id, token, expires_str, ip_address, user_agent);

        Json::Value ret;
        ret["status"] = "success";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        
        drogon::Cookie cookie("session_id", token);
        cookie.setHttpOnly(true);
        cookie.setSecure(true);
        cookie.setSameSite(drogon::Cookie::SameSite::kStrict);
        cookie.setPath("/");
        resp->addCookie(cookie);

        co_return resp;

    } catch (const drogon::orm::DrogonDbException &e) {
        std::println(stderr, "DB Error in login: {}", e.base().what());
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::logout(drogon::HttpRequestPtr req) {
    auto session_cookie = req->getCookie("session_id");
    if (!session_cookie.empty()) {
        auto db = drogon::app().getDbClient();
        try {
            co_await db->execSqlCoro("DELETE FROM sessions WHERE session_token = $1", session_cookie);
        } catch (...) {
            // ignore
        }
    }

    Json::Value ret;
    ret["status"] = "success";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    
    drogon::Cookie cookie("session_id", "");
    cookie.setExpiresDate(trantor::Date::date().after(-3600)); // expire immediately
    cookie.setPath("/");
    resp->addCookie(cookie);

    co_return resp;
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::me(drogon::HttpRequestPtr req) {
    auto session_cookie = req->getCookie("session_id");
    if (session_cookie.empty()) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k401Unauthorized);
        co_return resp;
    }

    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro(
            "SELECT u.id, u.loginname, u.email FROM users u "
            "JOIN sessions s ON u.id = s.user_id "
            "WHERE s.session_token = $1 AND s.expires_at > CURRENT_TIMESTAMP", 
            session_cookie
        );

        if (res.empty()) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            co_return resp;
        }

        Json::Value ret;
        ret["id"] = res[0]["id"].as<std::string>();
        ret["loginname"] = res[0]["loginname"].as<std::string>();
        ret["email"] = res[0]["email"].as<std::string>();
        
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;

    } catch (const drogon::orm::DrogonDbException &e) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::totp_setup(drogon::HttpRequestPtr req) {
    auto session_cookie = req->getCookie("session_id");
    if (session_cookie.empty()) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k401Unauthorized);
        co_return resp;
    }

    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro("SELECT user_id FROM sessions WHERE session_token = $1 AND expires_at > CURRENT_TIMESTAMP", session_cookie);
        if (res.empty()) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            co_return resp;
        }

        std::string user_id = res[0]["user_id"].as<std::string>();
        std::string secret = AuthSrv::generate_totp_secret();
        
        auto existing = co_await db->execSqlCoro("SELECT id FROM totp_secrets WHERE user_id = $1", user_id);
        if (existing.empty()) {
            co_await db->execSqlCoro("INSERT INTO totp_secrets (id, user_id, secret, issuer) VALUES ($1, $2, $3, $4)",
                drogon::utils::getUuid(), user_id, secret, "PhotoGallery");
        } else {
            co_await db->execSqlCoro("UPDATE totp_secrets SET secret = $1 WHERE user_id = $2", secret, user_id);
        }

        Json::Value ret;
        ret["secret"] = secret;
        ret["otpauth_uri"] = "otpauth://totp/PhotoGallery?secret=" + secret + "&issuer=PhotoGallery";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;

    } catch (...) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::totp_verify(drogon::HttpRequestPtr req) {
    auto json = req->getJsonObject();
    auto session_cookie = req->getCookie("session_id");
    if (!json || !json->isMember("code") || session_cookie.empty()) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        co_return resp;
    }
    
    std::string code = (*json)["code"].asString();
    
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro("SELECT user_id FROM sessions WHERE session_token = $1 AND expires_at > CURRENT_TIMESTAMP", session_cookie);
        if (res.empty()) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            co_return resp;
        }
        std::string user_id = res[0]["user_id"].as<std::string>();
        
        auto totp_res = co_await db->execSqlCoro("SELECT secret FROM totp_secrets WHERE user_id = $1", user_id);
        if (totp_res.empty()) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            co_return resp;
        }
        
        std::string secret = totp_res[0]["secret"].as<std::string>();
        if (AuthSrv::verify_totp(secret, code)) {
            // mark user as totp verified in session or users table if necessary
            Json::Value ret;
            ret["status"] = "success";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
            co_return resp;
        } else {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            co_return resp;
        }
    } catch (...) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::change_password(drogon::HttpRequestPtr req) {
    auto json = req->getJsonObject();
    auto session_cookie = req->getCookie("session_id");
    if (!json || !json->isMember("old_password") || !json->isMember("new_password") || session_cookie.empty()) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        co_return resp;
    }
    
    std::string old_pw = (*json)["old_password"].asString();
    std::string new_pw = (*json)["new_password"].asString();
    
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro(
            "SELECT u.id, u.password_hash FROM users u "
            "JOIN sessions s ON u.id = s.user_id "
            "WHERE s.session_token = $1 AND s.expires_at > CURRENT_TIMESTAMP", 
            session_cookie
        );

        if (res.empty()) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            co_return resp;
        }
        
        std::string hash = res[0]["password_hash"].as<std::string>();
        std::string user_id = res[0]["id"].as<std::string>();
        
        if (!AuthSrv::verify_password(old_pw, hash)) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            co_return resp;
        }
        
        auto new_hash = AuthSrv::hash_password(new_pw);
        if (!new_hash) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            co_return resp;
        }
        
        co_await db->execSqlCoro("UPDATE users SET password_hash = $1 WHERE id = $2", new_hash.value(), user_id);
        
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

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::reset_password_request(drogon::HttpRequestPtr req) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("email")) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        co_return resp;
    }
    
    std::string email = (*json)["email"].asString();
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro("SELECT id FROM users WHERE email = $1", email);
        if (!res.empty()) {
            std::string user_id = res[0]["id"].as<std::string>();
            std::string token = AuthSrv::generate_session_token(); // generate random string
            
            auto expires_at = trantor::Date::date().after(3600); // 1 hour validity
            
            co_await db->execSqlCoro(
                "INSERT INTO password_resets (id, user_id, token, expires_at) VALUES ($1, $2, $3, $4)",
                drogon::utils::getUuid(), user_id, token, expires_at.toDbStringLocal()
            );
            
            // In a real app we'd email this token. Here we return it for testing.
            Json::Value ret;
            ret["status"] = "success";
            ret["token"] = token; // MOCK ONLY
            auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
            co_return resp;
        }
        
        // Prevent user enumeration by returning success anyway
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

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::reset_password_confirm(drogon::HttpRequestPtr req) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("token") || !json->isMember("new_password")) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        co_return resp;
    }
    
    std::string token = (*json)["token"].asString();
    std::string new_pw = (*json)["new_password"].asString();
    
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro(
            "SELECT user_id FROM password_resets WHERE token = $1 AND expires_at > CURRENT_TIMESTAMP AND used = 0", 
            token
        );
        
        if (res.empty()) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            co_return resp;
        }
        
        std::string user_id = res[0]["user_id"].as<std::string>();
        auto new_hash = AuthSrv::hash_password(new_pw);
        
        auto trans = co_await db->newTransactionCoro();
        co_await trans->execSqlCoro("UPDATE users SET password_hash = $1 WHERE id = $2", new_hash.value(), user_id);
        co_await trans->execSqlCoro("UPDATE password_resets SET used = 1 WHERE token = $1", token);
        
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

} // namespace drogon_auth
