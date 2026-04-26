/**
 * SPDX-FileComment: Project Configuration template
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: MIT
 *
 * @file rz_config.hpp.in
 * @brief Configuration template for CMake.
 * @version 2.1.0
 * @date 2026-01-01
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license MIT License
 */
#pragma once

#include <string_view>

namespace rz {
namespace config {
constexpr std::string_view PROJECT_NAME = "drogon_auth";
constexpr std::string_view PROG_LONGNAME = "Drogon Authentication Microservice";
constexpr std::string_view PROJECT_DESCRIPTION = "A high-performance C++23 Drogon-based Authentication Microservice";

constexpr std::string_view EXECUTABLE_NAME = "drogon_auth";

constexpr std::string_view VERSION = "0.2.0";
constexpr std::int32_t PROJECT_VERSION_MAJOR { 0 };
constexpr std::int32_t PROJECT_VERSION_MINOR { 2 };
constexpr std::int32_t PROJECT_VERSION_PATCH { 0 };

constexpr std::string_view PROJECT_HOMEPAGE_URL = "https://github.com/zheng-bote/drogon_auth";
constexpr std::string_view AUTHOR = "ZHENG Bote";

constexpr std::string_view CREATED_YEAR = "2026";
constexpr std::string_view COPYRIGHT = "ZHENG Robert";
constexpr std::string_view LICENSE = "Apache-2.0";

constexpr std::string_view ORGANIZATION = "ZHENG Robert";
constexpr std::string_view PROJECT_DOMAIN = "net.hase-zheng";

constexpr std::string_view CMAKE_CXX_STANDARD = "c++23";
constexpr std::string_view CMAKE_CXX_COMPILER =
    "GNU 15.2.0";
constexpr std::string_view QT_VERSION_BUILD = "";
} // namespace config
} // namespace rz
