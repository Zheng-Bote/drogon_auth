/**
 * SPDX-FileComment: Authentication Controller
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file auth_ctrl.hpp
 * @brief Authentication Controller
 * @version 0.2.0
 * @date 2026-04-26
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#pragma once

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>
#include "auth_middleware.hpp"

namespace drogon_auth {

class AuthCtrl : public drogon::HttpController<AuthCtrl> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthCtrl::register_user, "/api/auth/v1/register", drogon::Post);
    ADD_METHOD_TO(AuthCtrl::login, "/api/auth/v1/login", drogon::Post);
    ADD_METHOD_TO(AuthCtrl::login_totp, "/api/auth/v1/login/totp", drogon::Post);
    ADD_METHOD_TO(AuthCtrl::logout, "/api/auth/v1/logout", drogon::Post, "drogon_auth::middleware::AuthMiddleware");
    ADD_METHOD_TO(AuthCtrl::me, "/api/auth/v1/me", drogon::Get, "drogon_auth::middleware::AuthMiddleware");
    ADD_METHOD_TO(AuthCtrl::totp_setup, "/api/auth/v1/totp/setup", drogon::Post, "drogon_auth::middleware::AuthMiddleware");
    ADD_METHOD_TO(AuthCtrl::totp_verify, "/api/auth/v1/totp/verify", drogon::Post, "drogon_auth::middleware::AuthMiddleware");
    ADD_METHOD_TO(AuthCtrl::change_password, "/api/auth/v1/password/change", drogon::Post, "drogon_auth::middleware::AuthMiddleware");
    ADD_METHOD_TO(AuthCtrl::reset_password_request, "/api/auth/v1/password/reset-request", drogon::Post);
    ADD_METHOD_TO(AuthCtrl::reset_password_confirm, "/api/auth/v1/password/reset-confirm", drogon::Post);
    ADD_METHOD_TO(AuthCtrl::get_profile, "/api/auth/v1/profile", drogon::Get, "drogon_auth::middleware::AuthMiddleware");
    ADD_METHOD_TO(AuthCtrl::update_profile, "/api/auth/v1/profile", drogon::Put, "drogon_auth::middleware::AuthMiddleware");
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> register_user(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> login(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> login_totp(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> logout(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> me(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> totp_setup(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> totp_verify(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> change_password(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> reset_password_request(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> reset_password_confirm(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> get_profile(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> update_profile(drogon::HttpRequestPtr req);
};

} // namespace drogon_auth