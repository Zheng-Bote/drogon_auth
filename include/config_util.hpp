/**
 * SPDX-FileComment: Configuration Utility
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file config_util.hpp
 * @brief Configuration Utility
 * @version 0.1.0
 * @date 2026-04-24
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#pragma once

#include <string>
#include <optional>

namespace drogon_auth {

/**
 * @class ConfigUtil
 * @brief Utility for reading configuration from .env files or environment variables.
 */
class ConfigUtil {
public:
    /**
     * @brief Loads the .env file from the given path.
     * @param path The path to the .env file.
     * @return true if loaded successfully, false otherwise.
     */
    static bool load_env(const std::string& path = ".env");

    /**
     * @brief Gets a configuration string value.
     * @param key The environment variable key.
     * @param default_val Fallback value if key is not found.
     * @return The configuration value.
     */
    static std::string get_string(const std::string& key, const std::string& default_val = "");

    /**
     * @brief Gets a configuration integer value.
     * @param key The environment variable key.
     * @param default_val Fallback value if key is not found.
     * @return The configuration value.
     */
    static int get_int(const std::string& key, int default_val = 0);
};

} // namespace drogon_auth
