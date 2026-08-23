/**
 * SPDX-FileComment: Authentication Controller Implementation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */
#include "auth_ctrl.hpp"
#include "auth_srv.hpp"
#include "db/auth_repository.hpp"
#include "utils/password_utils.hpp"
#include <drogon/utils/Utilities.h>
#include <drogon/plugins/AuditLogPlugin.hpp>
#include "cache/session_cache.hpp"
#include "metrics_registry.hpp"
#include "utils/rate_limiter.hpp"
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

    std::string user_id = drogon::utils::getUuid();
    bool success = co_await db::AuthRepository::create_user(user_id, loginname, email, hash_result.value());
    
    if (success) {
        Json::Value ret;
        ret["status"] = "success";
        ret["message"] = "User registered successfully";
        co_return drogon::HttpResponse::newHttpJsonResponse(ret);
    } else {
        std::println(stderr, "DB Error in register_user");
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

    // Rate Limiting (e.g. max 5 requests per 60 seconds)
    if (!utils::RateLimiter::instance().is_allowed(ip_address, 5, 60)) {
        co_return newJsonErrorResponse(drogon::k429TooManyRequests, "Too many login attempts. Please try again later.");
    }

    auto user_opt = co_await db::AuthRepository::find_user_by_login_or_email(ident);
    if (!user_opt || !user_opt->is_active) {
        if (user_opt) {
            co_await db::AuthRepository::record_login_attempt(user_opt->id, ident, ip_address, false);
        }
        co_return newJsonErrorResponse(drogon::k401Unauthorized, "Invalid credentials or account inactive");
    }

    auto user = user_opt.value();
    auto audit = drogon::app().getPlugin<drogon::plugins::AuditLogPlugin>();

    if (!AuthSrv::verify_password(password, user.password_hash)) {
        co_await db::AuthRepository::record_login_attempt(user.id, ident, ip_address, false);
        if (audit) audit->log(user.id, "login_failure", ip_address, Json::Value());
        metrics::MetricsRegistry::instance().record_auth_event(user.id, "login_failure", ip_address);
        co_return newJsonErrorResponse(drogon::k401Unauthorized, "Invalid credentials");
    }

    if (user.must_pwd_change) {
        Json::Value ret;
        ret["status"] = "password_change_required";
        ret["user_id"] = user.id;
        co_return drogon::HttpResponse::newHttpJsonResponse(ret);
    }

    bool totp_enabled = co_await db::AuthRepository::check_totp_enabled(user.id);
    if (totp_enabled) {
        if (audit) audit->log(user.id, "mfa_required", ip_address, Json::Value());
        metrics::MetricsRegistry::instance().record_auth_event(user.id, "mfa_required", ip_address);
        Json::Value ret;
        ret["status"] = "mfa_required";
        ret["user_id"] = user.id;
        co_return drogon::HttpResponse::newHttpJsonResponse(ret);
    }

    co_await db::AuthRepository::record_login_attempt(user.id, ident, ip_address, true);
    if (audit) audit->log(user.id, "login_success", ip_address, Json::Value());
    metrics::MetricsRegistry::instance().record_auth_event(user.id, "login_success", ip_address);

    std::string token = AuthSrv::generate_jwt_token(user.id);
    auto expires_at = trantor::Date::date().after(24 * 3600);
    
    // Sprint 7: Device-Awareness (Warnungen bei neuem Gerät)
    auto last_session_opt = co_await db::AuthRepository::get_last_session_for_user(user.id);
    if (!last_session_opt || (last_session_opt->ip_address != ip_address || last_session_opt->user_agent != req->getHeader("User-Agent"))) {
        if (audit) audit->log(user.id, "new_device_login", ip_address, Json::Value());
        metrics::MetricsRegistry::instance().record_auth_event(user.id, "new_device_login", ip_address);
    }

    std::string csrf_token = drogon::utils::getUuid();
    bool session_created = co_await db::AuthRepository::create_session(drogon::utils::getUuid(), user.id, token, expires_at.toDbStringLocal(), ip_address, req->getHeader("User-Agent"), csrf_token);
    if (!session_created) {
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Failed to create session");
    }

    cache::SessionMeta meta{user.id, ip_address, req->getHeader("User-Agent"), expires_at.toDbStringLocal(), csrf_token};
    cache::SessionCache::instance().put_session(token, meta);

    Json::Value ret;
    ret["status"] = "success";
    ret["csrf_token"] = csrf_token;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    
    drogon::Cookie cookie("JSESSIONID", token);
    cookie.setPath("/");
    cookie.setHttpOnly(true);
    cookie.setSecure(true);
    cookie.setSameSite(drogon::Cookie::SameSite::kStrict);
    resp->addCookie(cookie);

    auto session = req->session();
    if (session) {
        session->insert("authenticated", true);
        session->insert("user_id", user.id);
        session->insert("csrf_token", csrf_token);
    }

    co_return resp;
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::login_totp(drogon::HttpRequestPtr req) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("user_id") || !json->isMember("code")) {
        co_return newJsonErrorResponse(drogon::k400BadRequest, "Missing parameters");
    }

    std::string user_id = (*json)["user_id"].asString();
    std::string code = (*json)["code"].asString();
    std::string ip_address = req->peerAddr().toIp();

    if (!utils::RateLimiter::instance().is_allowed(ip_address, 5, 60)) {
        co_return newJsonErrorResponse(drogon::k429TooManyRequests, "Too many MFA attempts. Please try again later.");
    }

    auto secret_opt = co_await db::AuthRepository::get_totp_secret(user_id);
    if (!secret_opt) {
        co_return newJsonErrorResponse(drogon::k400BadRequest, "TOTP not enabled");
    }

    auto audit = drogon::app().getPlugin<drogon::plugins::AuditLogPlugin>();
    if (!AuthSrv::verify_totp(secret_opt.value(), code)) {
        if (audit) audit->log(user_id, "mfa_failure", ip_address, Json::Value());
        co_return newJsonErrorResponse(drogon::k401Unauthorized, "Invalid MFA code");
    }

    if (audit) audit->log(user_id, "login_success_mfa", ip_address, Json::Value());

    std::string token = AuthSrv::generate_jwt_token(user_id);
    auto expires_at = trantor::Date::date().after(24 * 3600);
    
    std::string csrf_token = drogon::utils::getUuid();
    co_await db::AuthRepository::create_session(drogon::utils::getUuid(), user_id, token, expires_at.toDbStringLocal(), ip_address, req->getHeader("User-Agent"), csrf_token);

    cache::SessionMeta meta{user_id, ip_address, req->getHeader("User-Agent"), expires_at.toDbStringLocal(), csrf_token};
    cache::SessionCache::instance().put_session(token, meta);

    Json::Value ret;
    ret["status"] = "success";
    ret["csrf_token"] = csrf_token;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    
    drogon::Cookie cookie("JSESSIONID", token);
    cookie.setPath("/");
    cookie.setHttpOnly(true);
    cookie.setSecure(true);
    cookie.setSameSite(drogon::Cookie::SameSite::kStrict);
    resp->addCookie(cookie);

    auto session = req->session();
    if (session) {
        session->insert("authenticated", true);
        session->insert("user_id", user_id);
        session->insert("csrf_token", csrf_token);
    }

    co_return resp;
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
        co_await db::AuthRepository::delete_session(session_cookie);
        cache::SessionCache::instance().remove_session(session_cookie);
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

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::refresh_session(drogon::HttpRequestPtr req) {
    auto session_cookie = req->getCookie("JSESSIONID");
    if (session_cookie.empty()) {
        co_return newJsonErrorResponse(drogon::k401Unauthorized, "No session");
    }

    auto session_opt = cache::SessionCache::instance().get_session(session_cookie);
    if (!session_opt) {
        co_return newJsonErrorResponse(drogon::k401Unauthorized, "Session expired or invalid");
    }
    
    auto expires_at = trantor::Date::date().after(24 * 3600);
    bool refreshed = co_await db::AuthRepository::refresh_session(session_cookie, expires_at.toDbStringLocal());
    
    if (refreshed) {
        if (session_opt) {
            auto meta = session_opt.value();
            meta.expires_at = expires_at.toDbStringLocal();
            cache::SessionCache::instance().put_session(session_cookie, meta);
        }

        Json::Value ret;
        ret["status"] = "success";
        ret["message"] = "Session refreshed";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        
        drogon::Cookie cookie("JSESSIONID", session_cookie);
        cookie.setPath("/");
        cookie.setHttpOnly(true);
        // Extend cookie expiration on client side
        // Typically JSESSIONID is session cookie, but if we want to extend we can set max_age
        resp->addCookie(cookie);
        
        co_return resp;
    } else {
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Failed to refresh session");
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::me(drogon::HttpRequestPtr req) {
    auto session_cookie = req->getCookie("JSESSIONID");
    if (session_cookie.empty()) {
        co_return newJsonErrorResponse(drogon::k401Unauthorized, "No session");
    }

    auto user_id_opt = co_await db::AuthRepository::get_user_id_by_session(session_cookie);
    if (!user_id_opt) {
        co_return newJsonErrorResponse(drogon::k401Unauthorized, "Session expired or invalid");
    }
    
    std::string user_id = user_id_opt.value();
    auto user_opt = co_await db::AuthRepository::find_user_by_id(user_id);
    if (!user_opt) {
        co_return newJsonErrorResponse(drogon::k404NotFound, "User not found");
    }

    bool totp_enabled = co_await db::AuthRepository::check_totp_enabled(user_id);
    auto last_login_opt = co_await db::AuthRepository::get_last_login_date(user_id);
    auto perms = co_await db::AuthRepository::get_user_permissions(user_id);
    auto roles = co_await db::AuthRepository::get_user_roles(user_id);

    Json::Value ret;
    ret["id"] = user_id;
    ret["loginname"] = user_opt->loginname;
    ret["email"] = user_opt->email;
    ret["two_factor_enabled"] = totp_enabled;
    ret["last_login"] = last_login_opt.value_or("");
    
    Json::Value roles_json(Json::arrayValue);
    for (const auto& r : roles) roles_json.append(r);
    ret["roles"] = roles_json;
    
    Json::Value perms_json(Json::arrayValue);
    for (const auto& p : perms) perms_json.append(p);
    ret["permissions"] = perms_json;

    co_return drogon::HttpResponse::newHttpJsonResponse(ret);
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::totp_setup(drogon::HttpRequestPtr req) {
    auto session_cookie = req->getCookie("JSESSIONID");
    if (session_cookie.empty()) co_return newJsonErrorResponse(drogon::k401Unauthorized, "Unauthorized");

    auto user_id_opt = co_await db::AuthRepository::get_user_id_by_session(session_cookie);
    if (!user_id_opt) co_return newJsonErrorResponse(drogon::k401Unauthorized, "Unauthorized");

    std::string user_id = user_id_opt.value();
    std::string secret = AuthSrv::generate_totp_secret();
    
    bool success = co_await db::AuthRepository::upsert_totp_secret(user_id, secret);
    if (!success) {
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Server error");
    }

    Json::Value ret;
    ret["secret"] = secret;
    ret["otpauth_uri"] = "otpauth://totp/Drogon%20Auth?secret=" + secret + "&issuer=Drogon%20Auth";
    co_return drogon::HttpResponse::newHttpJsonResponse(ret);
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::totp_verify(drogon::HttpRequestPtr req) {
    auto json = req->getJsonObject();
    auto session_cookie = req->getCookie("JSESSIONID");
    if (!json || !json->isMember("code") || session_cookie.empty()) {
        co_return newJsonErrorResponse(drogon::k400BadRequest, "Invalid request");
    }
    
    std::string code = (*json)["code"].asString();
    
    auto user_id_opt = co_await db::AuthRepository::get_user_id_by_session(session_cookie);
    if (!user_id_opt) co_return newJsonErrorResponse(drogon::k401Unauthorized, "Unauthorized");
    
    std::string user_id = user_id_opt.value();
    auto secret_opt = co_await db::AuthRepository::get_totp_secret(user_id);
    if (!secret_opt) co_return newJsonErrorResponse(drogon::k400BadRequest, "TOTP not setup");
    
    if (AuthSrv::verify_totp(secret_opt.value(), code)) {
        auto audit = drogon::app().getPlugin<drogon::plugins::AuditLogPlugin>();
        if (audit) audit->log(user_id, "totp_activated", req->peerAddr().toIp(), Json::Value());
        
        Json::Value ret;
        ret["status"] = "success";
        co_return drogon::HttpResponse::newHttpJsonResponse(ret);
    } else {
        co_return newJsonErrorResponse(drogon::k401Unauthorized, "Invalid TOTP code");
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
    
    auto user_id_opt = co_await db::AuthRepository::get_user_id_by_session(session_cookie);
    if (!user_id_opt) co_return newJsonErrorResponse(drogon::k401Unauthorized, "Unauthorized");
    
    std::string user_id = user_id_opt.value();
    auto user_opt = co_await db::AuthRepository::find_user_by_id(user_id);
    if (!user_opt) co_return newJsonErrorResponse(drogon::k404NotFound, "User not found");
    
    if (!AuthSrv::verify_password(old_pw, user_opt->password_hash)) {
        co_return newJsonErrorResponse(drogon::k401Unauthorized, "Current password incorrect");
    }
    
    auto new_hash = AuthSrv::hash_password(new_pw);
    if (!new_hash) co_return newJsonErrorResponse(drogon::k500InternalServerError, "Hashing error");
    
    if (co_await db::AuthRepository::update_password(user_id, new_hash.value())) {
        auto audit = drogon::app().getPlugin<drogon::plugins::AuditLogPlugin>();
        if (audit) audit->log(user_id, "password_changed", req->peerAddr().toIp(), Json::Value());

        Json::Value ret;
        ret["status"] = "success";
        co_return drogon::HttpResponse::newHttpJsonResponse(ret);
    } else {
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Server error");
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::change_password_forced(drogon::HttpRequestPtr req) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("user_id") || !json->isMember("old_password") || !json->isMember("new_password")) {
        co_return newJsonErrorResponse(drogon::k400BadRequest, "Invalid request");
    }
    
    std::string user_id = (*json)["user_id"].asString();
    std::string old_pw = (*json)["old_password"].asString();
    std::string new_pw = (*json)["new_password"].asString();
    
    auto user_opt = co_await db::AuthRepository::find_user_by_id(user_id);
    if (!user_opt) co_return newJsonErrorResponse(drogon::k404NotFound, "User not found");
    
    if (!user_opt->must_pwd_change) {
        co_return newJsonErrorResponse(drogon::k400BadRequest, "Password change not forced");
    }
    
    if (!AuthSrv::verify_password(old_pw, user_opt->password_hash)) {
        co_return newJsonErrorResponse(drogon::k401Unauthorized, "Current password incorrect");
    }
    
    auto new_hash = AuthSrv::hash_password(new_pw);
    if (!new_hash) co_return newJsonErrorResponse(drogon::k500InternalServerError, "Hashing error");
    
    if (co_await db::AuthRepository::update_password(user_id, new_hash.value())) {
        auto audit = drogon::app().getPlugin<drogon::plugins::AuditLogPlugin>();
        if (audit) audit->log(user_id, "password_changed_forced", req->peerAddr().toIp(), Json::Value());

        Json::Value ret;
        ret["status"] = "success";
        co_return drogon::HttpResponse::newHttpJsonResponse(ret);
    } else {
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Server error");
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::reset_password_request(drogon::HttpRequestPtr req) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("email")) co_return newJsonErrorResponse(drogon::k400BadRequest, "Email required");
    
    std::string email = (*json)["email"].asString();
    std::string ip_address = req->peerAddr().toIp();

    if (!utils::RateLimiter::instance().is_allowed(ip_address, 3, 300)) {
        co_return newJsonErrorResponse(drogon::k429TooManyRequests, "Too many password reset requests. Please try again later.");
    }

    auto user_id_opt = co_await db::AuthRepository::find_user_by_email(email);
    
    if (user_id_opt) {
        std::string token = drogon_auth::utils::PasswordUtils::generateRandomPassword(32);
        auto expires_at = trantor::Date::date().after(3600);
        
        co_await db::AuthRepository::create_password_reset(user_id_opt.value(), token, expires_at.toDbStringLocal());
        // TODO: Send token via Email Service
    }
    
    Json::Value ret;
    ret["status"] = "success";
    co_return drogon::HttpResponse::newHttpJsonResponse(ret);
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::reset_password_confirm(drogon::HttpRequestPtr req) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("token") || !json->isMember("new_password")) {
        co_return newJsonErrorResponse(drogon::k400BadRequest, "Invalid request");
    }
    
    std::string token = (*json)["token"].asString();
    std::string new_pw = (*json)["new_password"].asString();
    
    auto user_id_opt = co_await db::AuthRepository::find_user_by_reset_token(token);
    if (!user_id_opt) co_return newJsonErrorResponse(drogon::k400BadRequest, "Token invalid or expired");
    
    std::string user_id = user_id_opt.value();
    auto new_hash = AuthSrv::hash_password(new_pw);
    if (!new_hash) co_return newJsonErrorResponse(drogon::k500InternalServerError, "Hashing error");
    
    if (co_await db::AuthRepository::update_password(user_id, new_hash.value())) {
        co_await db::AuthRepository::mark_reset_token_used(token);
        auto audit = drogon::app().getPlugin<drogon::plugins::AuditLogPlugin>();
        if (audit) audit->log(user_id, "password_reset_confirm", req->peerAddr().toIp(), Json::Value());

        Json::Value ret;
        ret["status"] = "success";
        co_return drogon::HttpResponse::newHttpJsonResponse(ret);
    } else {
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Server error");
    }
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::get_profile(drogon::HttpRequestPtr req) {
    auto session = req->session();
    if (!session || !session->find("user_id")) co_return newJsonErrorResponse(drogon::k401Unauthorized, "Unauthorized");
    std::string user_id = session->get<std::string>("user_id");

    auto profile_opt = co_await db::AuthRepository::get_user_profile(user_id);
    if (!profile_opt) co_return newJsonErrorResponse(drogon::k404NotFound, "Profile not found");
    
    auto p = profile_opt.value();

    Json::Value ret;
    ret["first_name"] = p.first_name;
    ret["last_name"] = p.last_name;
    ret["preferred_language"] = p.preferred_language;
    ret["locale"] = p.locale;
    ret["timezone"] = p.timezone;
    ret["loginname"] = p.loginname;
    ret["email"] = p.email;
    ret["two_factor_enabled"] = p.two_factor_enabled;
    ret["last_password_change"] = p.last_password_change;

    Json::Value comms(Json::arrayValue);
    for (const auto& c : p.communications) {
        Json::Value jc;
        jc["channel"] = c.channel;
        jc["address"] = c.address;
        jc["is_active"] = c.is_active;
        jc["verified"] = c.verified;
        comms.append(jc);
    }
    ret["communications"] = comms;
    co_return drogon::HttpResponse::newHttpJsonResponse(ret);
}

drogon::Task<drogon::HttpResponsePtr> AuthCtrl::update_profile(drogon::HttpRequestPtr req) {
    auto session = req->session();
    if (!session || !session->find("user_id")) co_return newJsonErrorResponse(drogon::k401Unauthorized, "Unauthorized");
    std::string user_id = session->get<std::string>("user_id");

    auto json = req->getJsonObject();
    if (!json) co_return newJsonErrorResponse(drogon::k400BadRequest, "Invalid JSON");

    if (co_await db::AuthRepository::update_user_profile(user_id, *json)) {
        Json::Value ret;
        ret["status"] = "success";
        co_return drogon::HttpResponse::newHttpJsonResponse(ret);
    } else {
        co_return newJsonErrorResponse(drogon::k500InternalServerError, "Database error");
    }
}

} // namespace drogon_auth
