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
#include <argon2.h>
#include <random>
#include <vector>
#include <sstream>
#include <cstring>
#include <iomanip>

namespace drogon_auth {

std::expected<std::string, std::string> AuthSrv::hash_password(const std::string& password) {
    uint32_t t_cost = drogon_auth::utils::ConfigUtil::get_int("ARGON2_ITERATIONS", 3);
    uint32_t m_cost = drogon_auth::utils::ConfigUtil::get_int("ARGON2_MEMORY", 65536);
    uint32_t parallelism = drogon_auth::utils::ConfigUtil::get_int("ARGON2_PARALLELISM", 1);

    const uint32_t hash_len = 32;
    const uint32_t salt_len = 16;

    uint8_t salt[salt_len];
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint16_t> dis(0, 255);
    for (uint32_t i = 0; i < salt_len; ++i) {
        salt[i] = static_cast<uint8_t>(dis(gen));
    }

    size_t encoded_len = argon2_encodedlen(t_cost, m_cost, parallelism, salt_len, hash_len, Argon2_id);
    std::string encoded(encoded_len, '\0');

    int result = argon2id_hash_encoded(
        t_cost, m_cost, parallelism,
        password.c_str(), password.length(),
        salt, salt_len, hash_len,
        encoded.data(), encoded_len
    );

    if (result != ARGON2_OK) {
        return std::unexpected(argon2_error_message(result));
    }

    encoded.resize(strlen(encoded.c_str()));
    return encoded;
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