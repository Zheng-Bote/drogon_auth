/**
 * SPDX-FileComment: Admin Controller
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file admin_ctrl.hpp
 * @brief Admin Controller for User and Role Management
 * @version 0.1.0
 * @date 2026-05-01
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

class AdminCtrl : public drogon::HttpController<AdminCtrl> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AdminCtrl::list_users, "/api/admin/v1/users", drogon::Get, "drogon_auth::middleware::AuthMiddleware");
    ADD_METHOD_TO(AdminCtrl::create_user, "/api/admin/v1/users", drogon::Post, "drogon_auth::middleware::AuthMiddleware");
    ADD_METHOD_TO(AdminCtrl::update_user, "/api/admin/v1/users/{id}", drogon::Put, "drogon_auth::middleware::AuthMiddleware");
    ADD_METHOD_TO(AdminCtrl::delete_user, "/api/admin/v1/users/{id}", drogon::Delete, "drogon_auth::middleware::AuthMiddleware");
    ADD_METHOD_TO(AdminCtrl::list_roles, "/api/admin/v1/roles", drogon::Get, "drogon_auth::middleware::AuthMiddleware");
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> list_users(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> create_user(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> update_user(drogon::HttpRequestPtr req, std::string user_id);
    drogon::Task<drogon::HttpResponsePtr> delete_user(drogon::HttpRequestPtr req, std::string user_id);
    drogon::Task<drogon::HttpResponsePtr> list_roles(drogon::HttpRequestPtr req);

private:
    drogon::Task<bool> is_admin(const drogon::HttpRequestPtr& req);
};

} // namespace drogon_auth
