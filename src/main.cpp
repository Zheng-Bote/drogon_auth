/**
 * SPDX-FileComment: Main entry point
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file main.cpp
 * @brief Main entry point
 * @version 0.1.3
 * @date 2026-04-28
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#include "utils/config_utils.hpp"
#include "utils/seeder_utils.hpp"
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <string>
#include <trantor/utils/Logger.h>

int main(int argc, char *argv[]) {
  // 1. Determine environment
  std::string env_path = ".env";
  if (argc > 1) {
    env_path = argv[1];
  }
  drogon_auth::utils::ConfigUtil::load_env(env_path);

  // 2. Load configuration
  std::string drogon_config =
      drogon_auth::utils::ConfigUtil::get_string("DROGON_CONFIG_FILE", "");
  if (!drogon_config.empty()) {
    drogon::app().loadConfigFile(drogon_config);
  }

  // Collect variables for Logger-Callback
  int port = drogon_auth::utils::ConfigUtil::get_int("SERVER_PORT", 8848);
  std::string db_type =
      drogon_auth::utils::ConfigUtil::get_string("DB_TYPE", "postgres");

  // 3. Beginning Advice: Ensures logs end up in the FILE
  drogon::app().registerBeginningAdvice(
      [env_path, drogon_config, port, db_type]() {
        LOG_INFO << "--- Drogon Auth Microservice starting ---";
        LOG_INFO << "Environment file: " << env_path;
        if (!drogon_config.empty()) {
          LOG_INFO << "Drogon configuration: " << drogon_config;
        } else {
          LOG_INFO << "Drogon configuration: (internal defaults)";
        }
        LOG_INFO << "Server port: " << port;
        LOG_INFO << "Database type: " << db_type;

        // Seeder: Ensure at least one admin exists
        // async_run expects a callable returning an awaitable
        drogon::async_run([]() -> drogon::Task<void> {
          co_await drogon_auth::utils::Seeder::ensureAdminExists();
        });

        LOG_INFO << "--- Framework ready ---";
      });

  // 4. Initialize Database
  if (db_type == "postgres") {
    std::string db_host =
        drogon_auth::utils::ConfigUtil::get_string("DB_HOST", "127.0.0.1");
    int db_port = drogon_auth::utils::ConfigUtil::get_int("DB_PORT", 5432);
    std::string db_name =
        drogon_auth::utils::ConfigUtil::get_string("DB_NAME", "postgres");
    std::string db_user =
        drogon_auth::utils::ConfigUtil::get_string("DB_USER", "postgres");
    std::string db_password =
        drogon_auth::utils::ConfigUtil::get_string("DB_PASSWORD", "");

    drogon::app().createDbClient("postgresql", db_host, db_port, db_name,
                                 db_user, db_password, 1, "", "default");
  } else if (db_type == "sqlite3") {
    std::string sqlite_file = drogon_auth::utils::ConfigUtil::get_string(
        "SQLITE_FILE", "gallery.sqlite3");
    drogon::app().createDbClient("sqlite3", "", 0, "", "", "", 1, sqlite_file,
                                 "default");
  }

  LOG_INFO << "Drogon Auth Microservice starting on port " << port;
  drogon::app().addListener("0.0.0.0", port);

  // 5. Run the application
  drogon::app().run();

  LOG_INFO << "--- Drogon Auth Microservice stopped ---";

  return 0;
}
