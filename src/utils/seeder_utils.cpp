/**
 * SPDX-FileComment: Seeder Utility Implementation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file seeder_utils.cpp
 * @brief Seeder Utility Implementation
 * @version 0.1.0
 * @date 2026-04-28
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#include "utils/seeder_utils.hpp"
#include "utils/config_utils.hpp"
#include "auth_srv.hpp"
#include <drogon/drogon.h>
#include <drogon/utils/Utilities.h>

namespace drogon_auth::utils {

drogon::Task<void> Seeder::ensureAdminExists() {
    auto db = drogon::app().getDbClient();

    try {
        // 1. Check if any user with the 'admin' role exists
        auto res = co_await db->execSqlCoro(
            "SELECT COUNT(*) as count FROM user_roles ur "
            "JOIN roles r ON ur.role_id = r.id WHERE r.name = 'admin'"
        );

        if (res[0]["count"].as<long long>() > 0) {
            LOG_INFO << "System Check: Admin account already exists.";
            co_return;
        }

        LOG_WARN << "System Check: No admin found. Creating initial admin...";

        // 2. Get credentials from environment or defaults
        std::string adminUser = ConfigUtil::get_string("ADMIN_USER", "admin");
        std::string adminEmail = ConfigUtil::get_string("ADMIN_EMAIL", "admin@example.com");
        std::string adminPw = ConfigUtil::get_string("ADMIN_PASSWORD", "admin123");

        // 3. Hash the password
        auto hash_result = AuthSrv::hash_password(adminPw);
        if (!hash_result) {
            LOG_ERROR << "ERROR: Could not hash initial admin password!";
            co_return;
        }

        // 4. Start transaction to ensure atomicity
        auto trans = co_await db->newTransactionCoro();
        std::string userId = drogon::utils::getUuid();

        // 5. Insert Admin User
        co_await trans->execSqlCoro(
            "INSERT INTO users (id, loginname, email, password_hash, is_active) VALUES ($1, $2, $3, $4, $5)",
            userId, adminUser, adminEmail, hash_result.value(), true
        );

        // 6. Assign 'admin' role
        auto roleRes = co_await trans->execSqlCoro("SELECT id FROM roles WHERE name = 'admin'");
        if (roleRes.empty()) {
            LOG_ERROR << "ERROR: Role 'admin' not found in database! Make sure migrations were run.";
            // Transaction will rollback on destruction if not committed
            co_return;
        }

        std::string roleId = roleRes[0]["id"].as<std::string>();
        co_await trans->execSqlCoro(
            "INSERT INTO user_roles (user_id, role_id) VALUES ($1, $2)",
            userId, roleId
        );

        // 7. Create Profile
        co_await trans->execSqlCoro(
            "INSERT INTO user_profiles (id, user_id) VALUES ($1, $2)",
            drogon::utils::getUuid(), userId
        );

        LOG_INFO << "SUCCESS: Initial admin account and profile created.";
        LOG_INFO << "Login: " << adminUser << " / " << adminEmail;
    } catch (const std::exception &e) {
        LOG_ERROR << "Seeder Exception: " << e.what();
    } catch (...) {
        LOG_ERROR << "Seeder Exception: Unknown error occurred";
    }

    co_return;
}

} // namespace drogon_auth::utils
