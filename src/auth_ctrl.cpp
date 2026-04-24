/**
 * SPDX-FileComment: Authentication Controller Implementation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file auth_ctrl.cpp
 * @brief Authentication Controller Implementation
 * @version 0.1.0
 * @date 2026-04-24
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#include "auth_ctrl.hpp"
#include "auth_srv.hpp"
#include <print>

namespace drogon_auth {

void AuthCtrl::register_user(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }
    
    // TODO: Input validation (email format, password length)
    // TODO: Save to DB
    
    Json::Value ret;
    ret["status"] = "success";
    ret["message"] = "User registered successfully";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void AuthCtrl::login(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    // TODO: Fetch user from DB, verify password via AuthSrv::verify_password
    // TODO: Create session in DB, set secure HttpOnly cookie
    
    Json::Value ret;
    ret["status"] = "success";
    ret["token"] = AuthSrv::generate_session_token(); // Example
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void AuthCtrl::logout(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    // TODO: Invalidate session in DB
    Json::Value ret;
    ret["status"] = "success";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void AuthCtrl::me(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    // TODO: Verify session cookie, fetch user profile
    Json::Value ret;
    ret["email"] = "user@example.com";
    ret["totp_enabled"] = false;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void AuthCtrl::totp_setup(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    std::string secret = AuthSrv::generate_totp_secret();
    // TODO: Save secret temporarily to user profile or session
    Json::Value ret;
    ret["secret"] = secret;
    ret["otpauth_uri"] = "otpauth://totp/PhotoGallery:user@example.com?secret=" + secret + "&issuer=PhotoGallery";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void AuthCtrl::totp_verify(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("code")) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }
    
    std::string code = (*json)["code"].asString();
    // TODO: Get secret from DB for user
    std::string secret = "DUMMYSECRET"; 
    
    if (AuthSrv::verify_totp(secret, code)) {
        Json::Value ret;
        ret["status"] = "success";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    } else {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
    }
}

} // namespace drogon_auth
