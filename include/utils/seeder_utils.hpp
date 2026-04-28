/**
 * SPDX-FileComment: Seeder Utility for system initialization
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file seeder_utils.hpp
 * @brief Ensures initial system state like admin existence
 * @version 0.1.0
 * @date 2026-04-28
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#pragma once

#include <drogon/utils/coroutine.h>

namespace drogon_auth::utils {

/**
 * @class Seeder
 * @brief Handles initial database seeding and system consistency checks.
 */
class Seeder {
public:
    /**
     * @brief Ensures that at least one admin account exists in the system.
     * 
     * If no user with the 'admin' role is found, creates a default admin
     * account using environment variables or hardcoded defaults.
     * 
     * @return drogon::Task<void>
     */
    static drogon::Task<void> ensureAdminExists();
};

} // namespace drogon_auth::utils
