/**
 * SPDX-FileComment: Auth Middleware Implementation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file auth_middleware.cpp
 * @brief Auth Middleware Implementation
 * @version 0.2.0
 * @date 2026-04-28
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#include "auth_middleware.hpp"
#include <drogon/HttpResponse.h>
#include <json/json.h>

#include "cache/session_cache.hpp"
#include "db/auth_repository.hpp"
#include <drogon/utils/coroutine.h>

using namespace drogon_auth::middleware;

void AuthMiddleware::invoke(const drogon::HttpRequestPtr &req,
                            drogon::MiddlewareNextCallback &&nextCb,
                            drogon::MiddlewareCallback &&mcb) {
    auto session_cookie = req->getCookie("JSESSIONID");
    if (session_cookie.empty()) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k401Unauthorized);
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        Json::Value json;
        json["error"] = "Unauthorized";
        json["message"] = "No session cookie found";
        json["status"] = 401;
        resp->setBody(json.toStyledString());
        mcb(resp);
        return;
    }

    std::string ip = req->peerAddr().toIp();
    std::string user_agent = req->getHeader("User-Agent");

    // Sprint 6: In-Memory Session Cache read
    auto session_opt = drogon_auth::cache::SessionCache::instance().get_session(session_cookie);
    if (!session_opt) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k401Unauthorized);
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        Json::Value json;
        json["error"] = "Unauthorized";
        json["message"] = "Session expired or invalid";
        json["status"] = 401;
        resp->setBody(json.toStyledString());
        mcb(resp);
        return;
    }

    drogon::async_run([req, nextCb = std::move(nextCb), mcb = std::move(mcb), session_cookie, ip, user_agent, session_opt]() mutable -> drogon::Task<void> {
        try {

            auto session_data = session_opt.value();
            
            // Sprint 4: Re-Auth bei Client-Wechsel (Fingerprint-Vergleich)
            if (session_data.ip_address != ip || session_data.user_agent != user_agent) {
                // Delete compromised/invalid session
                co_await drogon_auth::db::AuthRepository::delete_session(session_cookie);
                
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k401Unauthorized);
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                Json::Value json;
                json["error"] = "Unauthorized";
                json["message"] = "Client fingerprint mismatch. Re-authentication required.";
                json["status"] = 401;
                json["requires_reauth"] = true;
                resp->setBody(json.toStyledString());
                mcb(resp);
                co_return;
            }

            // Sprint 4: CSRF-Integration
            if (req->method() == drogon::Post || req->method() == drogon::Put || req->method() == drogon::Delete || req->method() == drogon::Patch) {
                std::string header_csrf = req->getHeader("X-CSRF-TOKEN");
                if (header_csrf.empty() || header_csrf != session_data.csrf_token) {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k403Forbidden);
                    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                    Json::Value json;
                    json["error"] = "Forbidden";
                    json["message"] = "CSRF token missing or invalid";
                    json["status"] = 403;
                    resp->setBody(json.toStyledString());
                    mcb(resp);
                    co_return;
                }
            }

            // Populate session
            auto session = req->session();
            if (session) {
                session->insert("authenticated", true);
                session->insert("user_id", session_data.user_id);
            }

            nextCb(std::move(mcb));
        } catch (...) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            mcb(resp);
        }
    });
}
