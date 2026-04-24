/**
 * SPDX-FileComment: System Controller
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

namespace drogon_auth {

/**
 * @class SystemCtrl
 * @brief HTTP Controller for System endpoints (health check, version, updates).
 */
class SystemCtrl : public drogon::HttpController<SystemCtrl> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(SystemCtrl::getVersion, "/api/system/getVersion", drogon::Get);
    ADD_METHOD_TO(SystemCtrl::healthCheck, "/api/system/health-check", drogon::Get);
    ADD_METHOD_TO(SystemCtrl::checkUpdate, "/api/system/check-update", drogon::Get);
    ADD_METHOD_TO(SystemCtrl::sysInfo, "/api/system/sys-info", drogon::Get);
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> getVersion(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> healthCheck(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> checkUpdate(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> sysInfo(drogon::HttpRequestPtr req);
};

} // namespace drogon_auth
