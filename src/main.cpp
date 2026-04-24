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
    
    drogon::app().addListener("0.0.0.0", port);
    drogon::app().run();

    return 0;
}
