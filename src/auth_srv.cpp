/**
 * SPDX-FileComment: Authentication Service Implementation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file auth_srv.cpp
 * @brief Authentication Service Implementation
 * @version 0.2.0
 * @date 2026-04-27
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#include "auth_srv.hpp"
#include "utils/config_utils.hpp"
#include "utils/password_utils.hpp"
#include "utils/totp_utils.hpp"
#include <random>
#include <vector>
#include <sstream>
#include <cstring>
#include <iomanip>

namespace drogon_auth {

std::expected<std::string, std::string> AuthSrv::hash_password(const std::string& password) {
    std::string hashed = drogon_auth::utils::PasswordUtils::hashPassword(password);
    if (hashed.empty()) {
        return std::unexpected("Password hashing failed");
    }
    return hashed;
}

bool AuthSrv::verify_password(const std::string& password, const std::string& hash) {
    return drogon_auth::utils::PasswordUtils::verifyPassword(password, hash);
}

std::string AuthSrv::generate_session_token() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;

    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << dis(gen) << dis(gen);
    return ss.str();
}

bool AuthSrv::verify_totp(const std::string& secret, const std::string& code) {
    return drogon_auth::utils::TotpUtils::validateCode(secret, code);
}

std::string AuthSrv::generate_totp_secret() {
    return drogon_auth::utils::TotpUtils::generateSecret();
}

} // namespace drogon_auth