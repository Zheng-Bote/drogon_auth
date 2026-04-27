/**
 * SPDX-FileComment: Configuration Utility
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file config_utils.hpp
 * @brief Configuration Utility
 * @version 0.2.0
 * @date 2026-04-27
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#pragma once

#include <string>
#include <optional>
#include <cstdlib>

namespace drogon_auth {
namespace utils {

class ConfigUtil {
public:
    static bool load_env(const std::string& path = ".env");
    static std::string get_string(const std::string& key, const std::string& default_val = "");
    static int get_int(const std::string& key, int default_val = 0);
};

} // namespace utils

using ConfigUtil = utils::ConfigUtil;

} // namespace drogon_auth