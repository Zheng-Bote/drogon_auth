/**
 * SPDX-FileComment: System Controller Implementation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file system_ctrl.cpp
 * @brief System Controller returns System information
 * @version 0.2.0
 * @date 2026-04-26
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#include "system_ctrl.hpp"
#include "rz_config.hpp"
#include <check_gh-update.hpp>
#include <print>

#include <chrono>
#include <format>

namespace drogon_auth {

// ### => UTILS ###
std::string getCurrentYear() {
    auto dt = std::chrono::system_clock::now();
    auto dt_seconds = std::chrono::floor<std::chrono::seconds>(dt);
    
   return std::format("{0:%Y}", dt);
}
// ### <= UTILS ###

drogon::Task<drogon::HttpResponsePtr> SystemCtrl::getVersion(drogon::HttpRequestPtr req) {
    Json::Value ret;
    ret["version"] = rz::config::VERSION.data();
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    co_return resp;
}

drogon::Task<drogon::HttpResponsePtr> SystemCtrl::healthCheck(drogon::HttpRequestPtr req) {
    Json::Value ret;
    ret["status"] = "ok";
    ret["server"] = rz::config::PROJECT_NAME.data();
    
    // Check DB status
    auto db = drogon::app().getDbClient();
    try {
        co_await db->execSqlCoro("SELECT 1");
        ret["database"] = "connected";
        ret["database_version"] = (co_await db->execSqlCoro("SELECT version()"))[0][0].as<std::string>();
    } catch (...) {
        ret["database"] = "disconnected";
        ret["status"] = "error";
    }
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    if (ret["status"] == "error") {
        resp->setStatusCode(drogon::k500InternalServerError);
    }
    co_return resp;
}

drogon::Task<drogon::HttpResponsePtr> SystemCtrl::checkUpdate(drogon::HttpRequestPtr req) {
    Json::Value ret;
    try {
        const std::string repo_url(rz::config::PROJECT_HOMEPAGE_URL);
        const std::string current_version(rz::config::VERSION);

        if (!repo_url.empty() && !current_version.empty()) {
            auto result = ghupdate::check_github_update(repo_url, current_version);
            ret["update_available"] = result.hasUpdate;
            ret["current_version"] = current_version;
            if (result.hasUpdate) {
                ret["latest_version"] = result.latestVersion;
                ret["download_url"] = repo_url;
            }
        } else {
            ret["error"] = "Repository URL or Version is empty in configuration";
        }
    } catch (const std::exception &ex) {
        ret["error"] = ex.what();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k500InternalServerError);
        co_return resp;
    }
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    co_return resp;
}

drogon::Task<drogon::HttpResponsePtr> SystemCtrl::sysInfo(drogon::HttpRequestPtr req) {
    Json::Value ret;
    std::string copyright = rz::config::CREATED_YEAR.data();
    std::string current_year = getCurrentYear();
    if(copyright.compare(current_year) != 0) {
        copyright.append("-" + current_year);
    }   
    copyright.append(" ");  
    copyright.append(rz::config::COPYRIGHT.data());

    ret["project_name"] = rz::config::PROJECT_NAME.data();
    ret["description"] = rz::config::PROJECT_DESCRIPTION.data();
    ret["version"] = rz::config::VERSION.data();
    ret["homepage_url"] = rz::config::PROJECT_HOMEPAGE_URL.data();
    ret["author"] = rz::config::AUTHOR.data();
    ret["copyright"] = copyright;
    ret["license"] = rz::config::LICENSE.data();
    ret["organization"] = rz::config::ORGANIZATION.data();
    ret["compiler"] = rz::config::CMAKE_CXX_COMPILER.data();
    ret["standard"] = rz::config::CMAKE_CXX_STANDARD.data();
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    co_return resp;
}

} // namespace drogon_auth
