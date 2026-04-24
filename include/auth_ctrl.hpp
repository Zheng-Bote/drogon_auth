/**
 * SPDX-FileComment: Authentication Controller
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file auth_ctrl.hpp
 * @brief Authentication Controller
 * @version 0.1.0
 * @date 2026-04-24
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#pragma once

#include <drogon/HttpController.h>

namespace drogon_auth {

/**
 * @class AuthCtrl
 * @brief HTTP Controller for Authentication endpoints.
 */
class AuthCtrl : public drogon::HttpController<AuthCtrl> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthCtrl::register_user, "/api/v1/register", drogon::Post);
    ADD_METHOD_TO(AuthCtrl::login, "/api/v1/login", drogon::Post);
    ADD_METHOD_TO(AuthCtrl::logout, "/api/v1/logout", drogon::Post);
    ADD_METHOD_TO(AuthCtrl::me, "/api/v1/me", drogon::Get);
    ADD_METHOD_TO(AuthCtrl::totp_setup, "/api/v1/totp/setup", drogon::Post);
    ADD_METHOD_TO(AuthCtrl::totp_verify, "/api/v1/totp/verify", drogon::Post);
    METHOD_LIST_END

    /**
     * @brief Register a new user.
     * @param req The HTTP request.
     * @param callback The HTTP response callback.
     */
    void register_user(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief Log in a user and create a session.
     * @param req The HTTP request.
     * @param callback The HTTP response callback.
     */
    void login(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief Log out the user and invalidate session.
     * @param req The HTTP request.
     * @param callback The HTTP response callback.
     */
    void logout(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief Get current user profile.
     * @param req The HTTP request.
     * @param callback The HTTP response callback.
     */
    void me(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief Setup TOTP for the logged in user.
     * @param req The HTTP request.
     * @param callback The HTTP response callback.
     */
    void totp_setup(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief Verify TOTP code.
     * @param req The HTTP request.
     * @param callback The HTTP response callback.
     */
    void totp_verify(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace drogon_auth
