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
#include <unistd.h>
#include "metrics_registry.hpp"

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
    
    auto redis = drogon::app().getRedisClient();
    if (redis) {
        ret["redis"] = "configured";
    } else {
        ret["redis"] = "disabled";
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

static const auto app_start_time = std::chrono::steady_clock::now();

long get_memory_usage_kb() {
    long rss = 0;
    FILE* fp = fopen("/proc/self/statm", "r");
    if (fp != NULL) {
        long size, resident, share, text, lib, data, dt;
        if (fscanf(fp, "%ld %ld %ld %ld %ld %ld %ld", &size, &resident, &share, &text, &lib, &data, &dt) == 7) {
            rss = resident * (sysconf(_SC_PAGESIZE) / 1024);
        }
        fclose(fp);
    }
    return rss;
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

    auto now = std::chrono::steady_clock::now();
    auto uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - app_start_time).count();
    
    ret["uptime_seconds"] = (Json::Int64)uptime_seconds;
    ret["memory_usage_kb"] = (Json::Int64)get_memory_usage_kb();
    
    auto db = drogon::app().getDbClient();
    std::string db_version = "unknown";
    try {
        db_version = (co_await db->execSqlCoro("SHOW server_version"))[0][0].as<std::string>();
    } catch (...) {}

    if (db_version == "unknown") {
        try {
            db_version = (co_await db->execSqlCoro("SELECT sqlite_version()"))[0][0].as<std::string>();
        } catch (...) {}
    }
    ret["database_version"] = db_version;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    co_return resp;
}

drogon::Task<drogon::HttpResponsePtr> SystemCtrl::metrics(drogon::HttpRequestPtr req) {
    auto db = drogon::app().getDbClient();
    int64_t active_sessions = 0;
    int64_t failed_logins = 0;
    int64_t total_users = 0;

    try {
        auto res_sessions = co_await db->execSqlCoro("SELECT COUNT(*) FROM sessions WHERE expires_at > CURRENT_TIMESTAMP");
        if (!res_sessions.empty()) active_sessions = res_sessions[0][0].as<int64_t>();

        auto res_logins = co_await db->execSqlCoro("SELECT COUNT(*) FROM login_attempts WHERE success = false");
        if (!res_logins.empty()) failed_logins = res_logins[0][0].as<int64_t>();

        auto res_users = co_await db->execSqlCoro("SELECT COUNT(*) FROM users");
        if (!res_users.empty()) total_users = res_users[0][0].as<int64_t>();
    } catch (...) {
        // ignore db errors in metrics, return what we have
    }

    std::string prometheus_data;
    
    prometheus_data += "# HELP drogon_auth_active_sessions Number of currently active sessions\n";
    prometheus_data += "# TYPE drogon_auth_active_sessions gauge\n";
    prometheus_data += "drogon_auth_active_sessions " + std::to_string(active_sessions) + "\n\n";

    prometheus_data += "# HELP drogon_auth_failed_logins Total number of failed login attempts\n";
    prometheus_data += "# TYPE drogon_auth_failed_logins counter\n";
    prometheus_data += "drogon_auth_failed_logins " + std::to_string(failed_logins) + "\n\n";

    prometheus_data += "# HELP drogon_auth_total_users Total number of registered users\n";
    prometheus_data += "# TYPE drogon_auth_total_users gauge\n";
    prometheus_data += "drogon_auth_total_users " + std::to_string(total_users) + "\n\n";

    prometheus_data += "# HELP drogon_auth_api_latency_ms_sum Total latency per endpoint\n";
    prometheus_data += "# TYPE drogon_auth_api_latency_ms_sum summary\n";
    
    auto latencies = drogon_auth::metrics::MetricsRegistry::instance().get_latencies();
    for (const auto& [path, entry] : latencies) {
        prometheus_data += "drogon_auth_api_latency_ms_sum{path=\"" + path + "\"} " + std::to_string(entry.sum_ms) + "\n";
        prometheus_data += "drogon_auth_api_latency_ms_count{path=\"" + path + "\"} " + std::to_string(entry.count) + "\n";
    }
    prometheus_data += "\n";

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k200OK);
    resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
    resp->setBody(prometheus_data);
    co_return resp;
}

} // namespace drogon_auth
