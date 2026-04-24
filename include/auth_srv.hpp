/**
 * SPDX-FileComment: Authentication Service
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file auth_srv.hpp
 * @brief Authentication Service
 * @version 0.1.0
 * @date 2026-04-24
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#pragma once

#include <string>
#include <expected>

namespace drogon_auth {

/**
 * @struct UserProfile
 * @brief User data transfer object.
 */
struct UserProfile {
    std::string id;
    std::string email;
    bool totp_enabled;
};

/**
 * @class AuthSrv
 * @brief Service layer for authentication logic.
 */
class AuthSrv {
public:
    /**
     * @brief Hashes a password using Argon2.
     * @param password Plaintext password.
     * @return std::expected containing hashed password or error string.
     */
    [[nodiscard]] static std::expected<std::string, std::string> hash_password(const std::string& password);

    /**
     * @brief Verifies a plaintext password against an Argon2 hash.
     * @param password Plaintext password.
     * @param hash The stored hash.
     * @return true if it matches, false otherwise.
     */
    [[nodiscard]] static bool verify_password(const std::string& password, const std::string& hash);

    /**
     * @brief Generates a secure random session token.
     * @return Base64 or Hex encoded session token.
     */
    [[nodiscard]] static std::string generate_session_token();

    /**
     * @brief Verifies a TOTP code.
     * @param secret The base32 TOTP secret.
     * @param code The user-provided 6-digit code.
     * @return true if valid.
     */
    [[nodiscard]] static bool verify_totp(const std::string& secret, const std::string& code);
    
    /**
     * @brief Generates a new TOTP secret.
     * @return Base32 encoded secret.
     */
    [[nodiscard]] static std::string generate_totp_secret();
};

} // namespace drogon_auth
