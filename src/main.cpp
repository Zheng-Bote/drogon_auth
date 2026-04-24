/**
 * SPDX-FileComment: Main entry point
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file main.cpp
 * @brief Main entry point
 * @version 0.1.0
 * @date 2026-04-24
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#include <drogon/drogon.h>
#include <print>
#include <format>
#include "config_util.hpp"

int main(int argc, char* argv[]) {
    std::string env_path = ".env";
    if (argc > 1) {
        env_path = argv[1];
    }
    
    const std::string explicit_dotenv = drogon_auth::ConfigUtil::get_string("DOTENV_PATH");
    if (!explicit_dotenv.empty()) {
        env_path = explicit_dotenv;
    }

    drogon_auth::ConfigUtil::load_env(env_path);
    int port = drogon_auth::ConfigUtil::get_int("SERVER_PORT", 8848);

    std::println("Starting Drogon Auth Microservice on port {}", port);

    // EXTEND: Initialize DB client (PostgreSQL/SQLite) based on DB_TYPE
    std::string db_type = drogon_auth::ConfigUtil::get_string("DB_TYPE", "postgres");
    std::string db_host = drogon_auth::ConfigUtil::get_string("DB_HOST", "127.0.0.1");
    int db_port = drogon_auth::ConfigUtil::get_int("DB_PORT", 5432);
    std::string db_name = drogon_auth::ConfigUtil::get_string("DB_NAME", "postgres");
    std::string db_user = drogon_auth::ConfigUtil::get_string("DB_USER", "postgres");
    std::string db_password = drogon_auth::ConfigUtil::get_string("DB_PASSWORD", "");
    
    if (db_type == "postgres") {
        drogon::app().createDbClient("postgresql", db_host, db_port, db_name, db_user, db_password, 1, "", "default");
        std::println("Initialized PostgreSQL client");
    } else if (db_type == "sqlite3") {
        std::string sqlite_file = drogon_auth::ConfigUtil::get_string("SQLITE_FILE", "gallery.sqlite3");
        drogon::app().createDbClient("sqlite3", "", 0, "", "", "", 1, sqlite_file, "default");
        std::println("Initialized SQLite3 client (File: {})", sqlite_file);
    } else {
        std::println(stderr, "Unsupported DB_TYPE: {}", db_type);
    }
    
    drogon::app().addListener("0.0.0.0", port);
    drogon::app().run();

    return 0;
}
