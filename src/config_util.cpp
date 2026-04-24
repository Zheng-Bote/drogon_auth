/**
 * SPDX-FileComment: Configuration Utility Implementation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file config_util.cpp
 * @brief Configuration Utility Implementation
 * @version 0.1.0
 * @date 2026-04-24
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#include "config_util.hpp"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <unordered_map>
#include <print>

namespace drogon_auth {

static std::unordered_map<std::string, std::string> g_env_vars;

bool ConfigUtil::load_env(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::println(stderr, "Warning: Could not open env file: {}", path);
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line.find('=') == std::string::npos) continue;
        auto pos = line.find('=');
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);
        g_env_vars[key] = val;
    }
    return true;
}

std::string ConfigUtil::get_string(const std::string& key, const std::string& default_val) {
    if (g_env_vars.contains(key)) {
        return g_env_vars[key];
    }
    const char* env_val = std::getenv(key.c_str());
    if (env_val != nullptr) {
        return std::string(env_val);
    }
    return default_val;
}

int ConfigUtil::get_int(const std::string& key, int default_val) {
    std::string val = get_string(key, "");
    if (val.empty()) return default_val;
    try {
        return std::stoi(val);
    } catch (...) {
        return default_val;
    }
}

} // namespace drogon_auth
