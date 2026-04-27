/**
 * SPDX-FileComment: Login Filter Implementation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file login_filter.cpp
 * @brief Login Filter Implementation
 * @version 0.2.0
 * @date 2026-04-27
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#include "login_filter.hpp"
#include <drogon/HttpResponse.h>

using namespace drogon::filter;

void LoginFilter::doFilter(const HttpRequestPtr& req,
                          FilterCallback&& fcb,
                          FilterChainCallback&& fccb) {
    std::string path = req->path();

    if (isPublicPath(path)) {
        fccb();
        return;
    }

    if (isAuthenticated(req)) {
        fccb();
        return;
    }

    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k401Unauthorized);
    resp->setContentTypeCode(CT_APPLICATION_JSON);

    Json::Value json;
    json["error"] = "Unauthorized";
    json["message"] = "Login required";

    std::string body = json.toStyledString();
    resp->setBody(body);

    fcb(resp);
}

bool LoginFilter::isPublicPath(const std::string& path) const {
    static const std::vector<std::string> publicPaths = {
        "/api/auth/v1/register",
        "/api/auth/v1/login",
        "/api/auth/v1/password/reset-request",
        "/api/auth/v1/password/reset-confirm",
        "/api/auth/system/getVersion",
        "/api/auth/system/health-check",
        "/api/auth/system/check-update"
    };

    for (const auto& publicPath : publicPaths) {
        if (path == publicPath) {
            return true;
        }
    }

    if (path == "/" || path == "/index.html" || 
        path.find("/css/") == 0 || path.find("/js/") == 0 || 
        path.find("/img/") == 0 || path.find("/public/") == 0) {
        return true;
    }

    return false;
}

bool LoginFilter::isAuthenticated(const HttpRequestPtr& req) const {
    auto cookie = req->getCookie("DROGON_AUTH_JSESSIONID");
    return !cookie.empty() && cookie.length() > 8;
}