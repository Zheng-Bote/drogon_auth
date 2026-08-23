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
#include <jwt-cpp/traits/open-source-parsers-jsoncpp/defaults.h>
#include <jwt-cpp/jwt.h>
#include <sodium.h>
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



std::string AuthSrv::generate_jwt_token(const std::string& user_id) {
    if (sodium_init() < 0) {
        throw std::runtime_error("libsodium initialization failed");
    }

    auto secret = drogon_auth::utils::ConfigUtil::get_string("JWT_SECRET", "default_insecure_secret");
    
    unsigned char rand_bytes[16];
    randombytes_buf(rand_bytes, sizeof(rand_bytes));
    std::stringstream ss;
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(rand_bytes[i]);
    }
    std::string jti = ss.str();

    auto token = jwt::create()
        .set_issuer("Drogon_Auth")
        .set_type("JWS")
        .set_subject(user_id)
        .set_id(jti)
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{24})
        .sign(jwt::algorithm::hs256{secret});

    return token;
}

bool AuthSrv::verify_totp(const std::string& secret, const std::string& code) {
    return drogon_auth::utils::TotpUtils::validateCode(secret, code);
}

std::string AuthSrv::generate_totp_secret() {
    return drogon_auth::utils::TotpUtils::generateSecret();
}

} // namespace drogon_auth