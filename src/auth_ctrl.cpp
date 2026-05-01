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
#include <drogon/plugins/AuditLogPlugin.hpp>
#include <print>

namespace drogon_auth {

static drogon::HttpResponsePtr newJsonErrorResponse(drogon::HttpStatusCode code, const std::string& message) {
    Json::Value err;
    err["error"] = (code == drogon::k401Unauthorized) ? "Unauthorized" : 
                   (code == drogon::k403Forbidden) ? "Forbidden" :
                   (code == drogon::k404NotFound) ? "NotFound" : "Error";
    err["message"] = message;
    err["status"] = code;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
    resp->setStatusCode(code);
    return resp;
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::register_user(drogon::HttpRequestPtr req) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("loginname") || !json->isMember("email") || !json->isMember("password")) {
        co_return newJsonErrorResponse(drogon::k400BadRequest, "Missing required fields");
    }

    std::string loginname = (*json)["loginname"].asString();
    std::string email = (*json)["email"].asString();
    std::string password = (*json)["password"].asString();

    auto hash_result = AuthSrv::hash_password(password);
    if (!hash_result) {
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Hashing failed");
    }

    auto db = drogon::app().getDbClient();
    try {
        std::string user_id = drogon::utils::getUuid();

        auto trans = co_await db->newTransactionCoro();
        co_await trans->execSqlCoro(
            "INSERT INTO users (id, loginname, email, password_hash) VALUES ($1, $2, $3, $4)",
            user_id, loginname, email, hash_result.value()
        );
        co_await trans->execSqlCoro("INSERT INTO user_profiles (id, user_id) VALUES ($1, $2)", drogon::utils::getUuid(), user_id);
        co_await trans->execSqlCoro("INSERT INTO user_communications (id, user_id, channel, address) VALUES ($1, $2, $3, $4)", 
            drogon::utils::getUuid(), user_id, "email", email);

        co_await trans->execSqlCoro("COMMIT");

        Json::Value ret;
        ret["status"] = "success";
        ret["message"] = "User registered successfully";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        co_return resp;

    } catch (const drogon::orm::DrogonDbException &e) {
        std::println(stderr, "DB Error in register_user: {}", e.base().what());
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Database error");
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::login(drogon::HttpRequestPtr req) {
    auto json = req->getJsonObject();
    if (!json || (!json->isMember("loginname") && !json->isMember("email")) || !json->isMember("password")) {
        co_return newJsonErrorResponse(drogon::k400BadRequest, "Credentials missing");
    }

    std::string ident = json->isMember("loginname") ? (*json)["loginname"].asString() : (*json)["email"].asString();
    std::string password = (*json)["password"].asString();
    std::string ip_address = req->peerAddr().toIp();

    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro("SELECT id, loginname, password_hash, is_active FROM users WHERE loginname = $1 OR email = $1", ident);
        
        if (res.empty() || !res[0]["is_active"].as<bool>()) {
            if (!res.empty()) {
                co_await db->execSqlCoro("INSERT INTO login_attempts (id, user_id, loginname, ip_address, success) VALUES ($1, $2, $3, CAST($4 AS INET), $5)",
                    drogon::utils::getUuid(), res[0]["id"].as<std::string>(), ident, ip_address, false);
            }
            co_return newJsonErrorResponse(drogon::k401Unauthorized, "Invalid credentials or account inactive");
        }

        std::string user_id = res[0]["id"].as<std::string>();
        std::string hash = res[0]["password_hash"].as<std::string>();

        auto audit = drogon::app().getPlugin<drogon::plugins::AuditLogPlugin>();

        if (!AuthSrv::verify_password(password, hash)) {
            co_await db->execSqlCoro("INSERT INTO login_attempts (id, user_id, loginname, ip_address, success) VALUES ($1, $2, $3, CAST($4 AS INET), $5)",
                drogon::utils::getUuid(), user_id, ident, ip_address, false);
            
            if (audit) audit->log(user_id, "login_failure", ip_address, Json::Value());
            co_return newJsonErrorResponse(drogon::k401Unauthorized, "Invalid credentials");
        }

        // Check if MFA (TOTP) is required
        auto totp_res = co_await db->execSqlCoro("SELECT 1 FROM totp_secrets WHERE user_id = $1", user_id);
        if (!totp_res.empty()) {
            if (audit) audit->log(user_id, "mfa_required", ip_address, Json::Value());
            
            Json::Value ret;
            ret["status"] = "mfa_required";
            ret["user_id"] = user_id; // Frontend needs this for the second step
            co_return drogon::HttpResponse::newHttpJsonResponse(ret);
        }

        // Success (No MFA)
        co_await db->execSqlCoro("INSERT INTO login_attempts (id, user_id, loginname, ip_address, success) VALUES ($1, $2, $3, CAST($4 AS INET), $5)",
            drogon::utils::getUuid(), user_id, ident, ip_address, true);
        
        if (audit) audit->log(user_id, "login_success", ip_address, Json::Value());

        std::string token = AuthSrv::generate_session_token();
        auto expires_at = trantor::Date::date().after(24 * 3600);
        std::string expires_str = expires_at.toDbStringLocal();

        co_await db->execSqlCoro("INSERT INTO sessions (id, user_id, session_token, expires_at, ip_address, user_agent) VALUES ($1, $2, $3, $4, CAST($5 AS INET), $6)",
            drogon::utils::getUuid(), user_id, token, expires_str, ip_address, req->getHeader("User-Agent"));

        Json::Value ret;
        ret["status"] = "success";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        
        drogon::Cookie cookie("JSESSIONID", token);
        cookie.setPath("/");
        cookie.setHttpOnly(true);
        resp->addCookie(cookie);

        auto session = req->session();
        if (session) {
            session->insert("authenticated", true);
            session->insert("user_id", user_id);
        }

        co_return resp;

    } catch (const std::exception &e) {
        std::println(stderr, "Error in login: {}", e.what());
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Server error");
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::login_totp(drogon::HttpRequestPtr req) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("user_id") || !json->isMember("code")) {
        co_return newJsonErrorResponse(drogon::k400BadRequest, "Missing parameters");
    }

    std::string user_id = (*json)["user_id"].asString();
    std::string code = (*json)["code"].asString();
    std::string ip_address = req->peerAddr().toIp();

    auto db = drogon::app().getDbClient();
    try {
        auto totp_res = co_await db->execSqlCoro("SELECT secret FROM totp_secrets WHERE user_id = $1", user_id);
        if (totp_res.empty()) {
            co_return newJsonErrorResponse(drogon::k400BadRequest, "TOTP not enabled");
        }

        std::string secret = totp_res[0]["secret"].as<std::string>();
        auto audit = drogon::app().getPlugin<drogon::plugins::AuditLogPlugin>();

        if (!AuthSrv::verify_totp(secret, code)) {
            if (audit) audit->log(user_id, "mfa_failure", ip_address, Json::Value());
            co_return newJsonErrorResponse(drogon::k401Unauthorized, "Invalid MFA code");
        }

        // MFA Success
        if (audit) audit->log(user_id, "login_success_mfa", ip_address, Json::Value());

        std::string token = AuthSrv::generate_session_token();
        auto expires_at = trantor::Date::date().after(24 * 3600);
        
        co_await db->execSqlCoro("INSERT INTO sessions (id, user_id, session_token, expires_at, ip_address, user_agent) VALUES ($1, $2, $3, $4, CAST($5 AS INET), $6)",
            drogon::utils::getUuid(), user_id, token, expires_at.toDbStringLocal(), ip_address, req->getHeader("User-Agent"));

        Json::Value ret;
        ret["status"] = "success";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        
        drogon::Cookie cookie("JSESSIONID", token);
        cookie.setPath("/");
        cookie.setHttpOnly(true);
        resp->addCookie(cookie);

        auto session = req->session();
        if (session) {
            session->insert("authenticated", true);
            session->insert("user_id", user_id);
        }

        co_return resp;

    } catch (const std::exception &e) {
        std::println(stderr, "Error in login_totp: {}", e.what());
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Server error");
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::logout(drogon::HttpRequestPtr req) {
    auto audit = drogon::app().getPlugin<drogon::plugins::AuditLogPlugin>();
    std::string ip_address = req->peerAddr().toIp();

    auto session = req->session();
    if (session) {
        if (audit && session->find("user_id")) {
            audit->log(session->get<std::string>("user_id"), "logout", ip_address, Json::Value());
        }
        session->erase("authenticated");
        session->erase("user_id");
    }

    auto session_cookie = req->getCookie("JSESSIONID");
    if (!session_cookie.empty()) {
        auto db = drogon::app().getDbClient();
        try {
            co_await db->execSqlCoro("DELETE FROM sessions WHERE session_token = $1", session_cookie);
        } catch (...) {}
    }

    Json::Value ret;
    ret["status"] = "success";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    
    drogon::Cookie cookie("JSESSIONID", "");
    cookie.setExpiresDate(trantor::Date::date().after(-3600));
    cookie.setPath("/");
    resp->addCookie(cookie);

    co_return resp;
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::me(drogon::HttpRequestPtr req) {
    auto session_cookie = req->getCookie("JSESSIONID");
    if (session_cookie.empty()) {
        co_return newJsonErrorResponse(drogon::k401Unauthorized, "No session");
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
            co_return newJsonErrorResponse(drogon::k401Unauthorized, "Session expired or invalid");
        }

        Json::Value ret;
        ret["id"] = res[0]["id"].as<std::string>();
        ret["loginname"] = res[0]["loginname"].as<std::string>();
        ret["email"] = res[0]["email"].as<std::string>();
        co_return drogon::HttpResponse::newHttpJsonResponse(ret);

    } catch (const drogon::orm::DrogonDbException &e) {
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Server error");
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::totp_setup(drogon::HttpRequestPtr req) {
    auto session_cookie = req->getCookie("JSESSIONID");
    if (session_cookie.empty()) {
        co_return newJsonErrorResponse(drogon::k401Unauthorized, "Unauthorized");
    }

    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro("SELECT user_id FROM sessions WHERE session_token = $1 AND expires_at > CURRENT_TIMESTAMP", session_cookie);
        if (res.empty()) co_return newJsonErrorResponse(drogon::k401Unauthorized, "Unauthorized");

        std::string user_id = res[0]["user_id"].as<std::string>();
        std::string secret = AuthSrv::generate_totp_secret();
        
        auto trans = co_await db->newTransactionCoro();
        auto existing = co_await trans->execSqlCoro("SELECT id FROM totp_secrets WHERE user_id = $1", user_id);
        if (existing.empty()) {
            co_await trans->execSqlCoro("INSERT INTO totp_secrets (id, user_id, secret, issuer) VALUES ($1, $2, $3, $4)",
                drogon::utils::getUuid(), user_id, secret, "PhotoGallery");
        } else {
            co_await trans->execSqlCoro("UPDATE totp_secrets SET secret = $1 WHERE user_id = $2", secret, user_id);
        }
        co_await trans->execSqlCoro("COMMIT");

        Json::Value ret;
        ret["secret"] = secret;
        ret["otpauth_uri"] = "otpauth://totp/PhotoGallery?secret=" + secret + "&issuer=PhotoGallery";
        co_return drogon::HttpResponse::newHttpJsonResponse(ret);

    } catch (...) {
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Server error");
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::totp_verify(drogon::HttpRequestPtr req) {
    auto json = req->getJsonObject();
    auto session_cookie = req->getCookie("JSESSIONID");
    if (!json || !json->isMember("code") || session_cookie.empty()) {
        co_return newJsonErrorResponse(drogon::k400BadRequest, "Invalid request");
    }
    
    std::string code = (*json)["code"].asString();
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro("SELECT user_id FROM sessions WHERE session_token = $1 AND expires_at > CURRENT_TIMESTAMP", session_cookie);
        if (res.empty()) co_return newJsonErrorResponse(drogon::k401Unauthorized, "Unauthorized");
        
        std::string user_id = res[0]["user_id"].as<std::string>();
        auto totp_res = co_await db->execSqlCoro("SELECT secret FROM totp_secrets WHERE user_id = $1", user_id);
        if (totp_res.empty()) co_return newJsonErrorResponse(drogon::k400BadRequest, "TOTP not setup");
        
        std::string secret = totp_res[0]["secret"].as<std::string>();
        if (AuthSrv::verify_totp(secret, code)) {
            auto audit = drogon::app().getPlugin<drogon::plugins::AuditLogPlugin>();
            if (audit) audit->log(user_id, "totp_activated", req->peerAddr().toIp(), Json::Value());
            
            Json::Value ret;
            ret["status"] = "success";
            co_return drogon::HttpResponse::newHttpJsonResponse(ret);
        } else {
            co_return newJsonErrorResponse(drogon::k401Unauthorized, "Invalid TOTP code");
        }
    } catch (...) {
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Server error");
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::change_password(drogon::HttpRequestPtr req) {
    auto json = req->getJsonObject();
    auto session_cookie = req->getCookie("JSESSIONID");
    if (!json || !json->isMember("old_password") || !json->isMember("new_password") || session_cookie.empty()) {
        co_return newJsonErrorResponse(drogon::k400BadRequest, "Invalid request");
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

        if (res.empty()) co_return newJsonErrorResponse(drogon::k401Unauthorized, "Unauthorized");
        
        std::string hash = res[0]["password_hash"].as<std::string>();
        std::string user_id = res[0]["id"].as<std::string>();
        
        if (!AuthSrv::verify_password(old_pw, hash)) {
            co_return newJsonErrorResponse(drogon::k401Unauthorized, "Current password incorrect");
        }
        
        auto new_hash = AuthSrv::hash_password(new_pw);
        if (!new_hash) co_return newJsonErrorResponse(drogon::k500InternalServerError, "Hashing error");
        
        co_await db->execSqlCoro("UPDATE users SET password_hash = $1 WHERE id = $2", new_hash.value(), user_id);
        
        auto audit = drogon::app().getPlugin<drogon::plugins::AuditLogPlugin>();
        if (audit) audit->log(user_id, "password_changed", req->peerAddr().toIp(), Json::Value());

        Json::Value ret;
        ret["status"] = "success";
        co_return drogon::HttpResponse::newHttpJsonResponse(ret);
    } catch (...) {
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Server error");
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::reset_password_request(drogon::HttpRequestPtr req) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("email")) co_return newJsonErrorResponse(drogon::k400BadRequest, "Email required");
    
    std::string email = (*json)["email"].asString();
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro("SELECT id FROM users WHERE email = $1", email);
        if (!res.empty()) {
            std::string user_id = res[0]["id"].as<std::string>();
            std::string token = AuthSrv::generate_session_token();
            auto expires_at = trantor::Date::date().after(3600);
            
            co_await db->execSqlCoro(
                "INSERT INTO password_resets (id, user_id, token, expires_at) VALUES ($1, $2, $3, $4)",
                drogon::utils::getUuid(), user_id, token, expires_at.toDbStringLocal()
            );
            
            Json::Value ret;
            ret["status"] = "success";
            ret["token"] = token; // MOCK ONLY
            co_return drogon::HttpResponse::newHttpJsonResponse(ret);
        }
        Json::Value ret;
        ret["status"] = "success";
        co_return drogon::HttpResponse::newHttpJsonResponse(ret);
        
    } catch (...) {
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Server error");
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::reset_password_confirm(drogon::HttpRequestPtr req) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("token") || !json->isMember("new_password")) co_return newJsonErrorResponse(drogon::k400BadRequest, "Invalid request");
    
    std::string token = (*json)["token"].asString();
    std::string new_pw = (*json)["new_password"].asString();
    
    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro(
            "SELECT user_id FROM password_resets WHERE token = $1 AND expires_at > CURRENT_TIMESTAMP AND used = 0", 
            token
        );
        
        if (res.empty()) co_return newJsonErrorResponse(drogon::k400BadRequest, "Token invalid or expired");
        
        std::string user_id = res[0]["user_id"].as<std::string>();
        auto new_hash = AuthSrv::hash_password(new_pw);
        
        auto trans = co_await db->newTransactionCoro();
        co_await trans->execSqlCoro("UPDATE users SET password_hash = $1 WHERE id = $2", new_hash.value(), user_id);
        co_await trans->execSqlCoro("UPDATE password_resets SET used = 1 WHERE token = $1", token);
        co_await trans->execSqlCoro("COMMIT");
        
        auto audit = drogon::app().getPlugin<drogon::plugins::AuditLogPlugin>();
        if (audit) audit->log(user_id, "password_reset_confirm", req->peerAddr().toIp(), Json::Value());

        Json::Value ret;
        ret["status"] = "success";
        co_return drogon::HttpResponse::newHttpJsonResponse(ret);
        
    } catch (...) {
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Server error");
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::get_profile(drogon::HttpRequestPtr req) {
    auto session = req->session();
    if (!session || !session->find("user_id")) co_return newJsonErrorResponse(drogon::k401Unauthorized, "Unauthorized");
    std::string user_id = session->get<std::string>("user_id");

    auto db = drogon::app().getDbClient();
    try {
        auto res = co_await db->execSqlCoro(
            "SELECT p.first_name, p.last_name, p.preferred_language, p.locale, p.timezone, u.loginname, u.email "
            "FROM user_profiles p JOIN users u ON p.user_id = u.id WHERE p.user_id = $1", 
            user_id
        );

        if (res.empty()) co_return newJsonErrorResponse(drogon::k404NotFound, "Profile not found");

        Json::Value ret;
        ret["first_name"] = res[0]["first_name"].as<std::string>();
        ret["last_name"] = res[0]["last_name"].as<std::string>();
        ret["preferred_language"] = res[0]["preferred_language"].as<std::string>();
        ret["locale"] = res[0]["locale"].as<std::string>();
        ret["timezone"] = res[0]["timezone"].as<std::string>();
        ret["loginname"] = res[0]["loginname"].as<std::string>();
        ret["email"] = res[0]["email"].as<std::string>();

        auto comm_res = co_await db->execSqlCoro("SELECT channel, address, is_active, verified FROM user_communications WHERE user_id = $1", user_id);
        Json::Value comms(Json::arrayValue);
        for (const auto& row : comm_res) {
            Json::Value c;
            c["channel"] = row["channel"].as<std::string>();
            c["address"] = row["address"].as<std::string>();
            c["is_active"] = row["is_active"].as<bool>();
            c["verified"] = row["verified"].as<bool>();
            comms.append(c);
        }
        ret["communications"] = comms;
        co_return drogon::HttpResponse::newHttpJsonResponse(ret);

    } catch (...) {
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Server error");
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::update_profile(drogon::HttpRequestPtr req) {
    auto session = req->session();
    if (!session || !session->find("user_id")) co_return newJsonErrorResponse(drogon::k401Unauthorized, "Unauthorized");
    std::string user_id = session->get<std::string>("user_id");

    auto json = req->getJsonObject();
    if (!json) co_return newJsonErrorResponse(drogon::k400BadRequest, "Invalid JSON");

    auto db = drogon::app().getDbClient();
    try {
        auto trans = co_await db->newTransactionCoro();
        
        co_await trans->execSqlCoro(
            "INSERT INTO user_profiles (id, user_id) VALUES ($1, $2) ON CONFLICT (user_id) DO NOTHING",
            drogon::utils::getUuid(), user_id
        );

        if (json->isMember("first_name")) co_await trans->execSqlCoro("UPDATE user_profiles SET first_name = $1 WHERE user_id = $2", (*json)["first_name"].asString(), user_id);
        if (json->isMember("last_name")) co_await trans->execSqlCoro("UPDATE user_profiles SET last_name = $1 WHERE user_id = $2", (*json)["last_name"].asString(), user_id);
        if (json->isMember("preferred_language")) co_await trans->execSqlCoro("UPDATE user_profiles SET preferred_language = $1 WHERE user_id = $2", (*json)["preferred_language"].asString(), user_id);
        if (json->isMember("timezone")) co_await trans->execSqlCoro("UPDATE user_profiles SET timezone = $1 WHERE user_id = $2", (*json)["timezone"].asString(), user_id);

        if (json->isMember("communications") && (*json)["communications"].isArray()) {
            co_await trans->execSqlCoro("DELETE FROM user_communications WHERE user_id = $1", user_id);
            for (const auto& c : (*json)["communications"]) {
                co_await trans->execSqlCoro(
                    "INSERT INTO user_communications (id, user_id, channel, address, is_active) VALUES ($1, $2, CAST($3 AS communication_channel), $4, $5)",
                    drogon::utils::getUuid(), user_id, c["channel"].asString(), c["address"].asString(), c.get("is_active", true).asBool()
                );
            }
        }
        co_await trans->execSqlCoro("COMMIT");

        Json::Value ret;
        ret["status"] = "success";
        co_return drogon::HttpResponse::newHttpJsonResponse(ret);

    } catch (const std::exception& e) {
        std::println(stderr, "Error in update_profile: {}", e.what());
        co_return newJsonErrorResponse(drogon::k500InternalServerError, std::string("Database error: ") + e.what());
    } catch (...) {
        std::println(stderr, "Unknown error in update_profile");
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Unknown server error");
    }
}

} // namespace drogon_auth
