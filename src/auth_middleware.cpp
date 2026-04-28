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

using namespace drogon_auth::middleware;

void AuthMiddleware::invoke(const drogon::HttpRequestPtr &req,
                            drogon::MiddlewareNextCallback &&nextCb,
                            drogon::MiddlewareCallback &&mcb) {

    auto session = req->session();
    
    // Check if session exists and has the "authenticated" flag
    if (session && session->getOptional<bool>("authenticated").value_or(false)) {
        nextCb(std::move(mcb));
        return;
    }

    // Fallback: If no session flag, return 401
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k401Unauthorized);
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);

    Json::Value json;
    json["error"] = "Unauthorized";
    json["message"] = "Login required to access this resource";
    json["status"] = 401;

    resp->setBody(json.toStyledString());
    mcb(resp);
}
