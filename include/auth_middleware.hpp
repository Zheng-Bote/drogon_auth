/**
 * SPDX-FileComment: Auth Middleware
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file auth_middleware.hpp
 * @brief Auth Middleware for protected routes
 * @version 0.1.0
 * @date 2026-04-28
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#pragma once

#include <drogon/HttpMiddleware.h>

namespace drogon_auth::middleware {

/**
 * @class AuthMiddleware
 * @brief Middleware to check if a user is authenticated.
 */
class AuthMiddleware : public drogon::HttpMiddleware<AuthMiddleware> {
public:
    AuthMiddleware() = default;

    void invoke(const drogon::HttpRequestPtr& req,
                drogon::MiddlewareNextCallback&& nextCb,
                drogon::MiddlewareCallback&& mcb) override;
};

} // namespace drogon_auth::middleware
