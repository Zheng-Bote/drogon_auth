/**
 * SPDX-FileComment: Authentication Service
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file auth_srv.hpp
 * @brief Authentication Service
 * @version 0.2.0
 * @date 2026-04-27
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#pragma once

#include <string>
#include <expected>
#include <cstdint>
#include <vector>

namespace drogon_auth {

struct UserProfile {
    std::string id;
    std::string loginname;
    std::string email;
    bool totp_enabled;
};

class AuthSrv {
public:
    [[nodiscard]] static std::expected<std::string, std::string> hash_password(const std::string& password);
    [[nodiscard]] static bool verify_password(const std::string& password, const std::string& hash);
    [[nodiscard]] static std::string generate_session_token();
    [[nodiscard]] static bool verify_totp(const std::string& secret, const std::string& code);
    [[nodiscard]] static std::string generate_totp_secret();
};

} // namespace drogon_auth