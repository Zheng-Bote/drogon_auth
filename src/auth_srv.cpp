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
 * @date 2026-04-26
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#include "auth_srv.hpp"
#include "config_util.hpp"
#include <argon2.h>
#include <random>
#include <vector>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <chrono>

// Base32 Alphabet (RFC 4648)
static const char *B32_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

namespace drogon_auth {

// ### Utils => ###

/**
 * @brief Gets the current time step for TOTP calculation.
 *
 * TOTP uses 30-second intervals. This function returns the number of 30-second
 * intervals that have elapsed since the Unix epoch.
 *
 * @return The current time step.
 */
 int64_t AuthSrv::getCurrentTimeStep() {
    //return QDateTime::currentSecsSinceEpoch() / 30;
    using namespace std::chrono;

  system_clock::time_point tp = system_clock::now();
  system_clock::duration dtn = tp.time_since_epoch();
  return dtn.count() * system_clock::period::num / system_clock::period::den / 30;
}

/**
 * @brief Decodes a Base32 string into bytes.
 *
 * @param secret The Base32 encoded secret.
 * @return A vector of bytes representing the decoded secret.
 */
 std::vector<uint8_t> AuthSrv::base32Decode(const std::string &secret) {
    std::vector<uint8_t> result;
    unsigned int buffer = 0;
    int bitsLeft = 0;

    for (unsigned char c : secret) {
        // C++23: to_uppercase ohne Locale
        c = std::toupper(c);

        const char* p = std::strchr(B32_CHARS, c);
        if (!p)
            continue;  // ignoriert ungültige Zeichen wie im Qt-Code

        buffer = (buffer << 5) | (p - B32_CHARS);
        bitsLeft += 5;

        if (bitsLeft >= 8) {
            result.push_back((buffer >> (bitsLeft - 8)) & 0xFF);
            bitsLeft -= 8;
        }
    }

    return result;
}

/**
 * @brief Generates a TOTP code for a specific time step.
 *
 * Calculates the HMAC-SHA1 hash of the time step using the secret key,
 * and extracts a 6-digit code.
 *
 * @param keyBytes The decoded secret key in bytes.
 * @param timeStep The time step to generate the code for.
 * @return The 6-digit TOTP code as a string.
 */
 std::string AuthSrv::generateCodeForStep(const std::vector<uint8_t> &keyBytes, int64_t timeStep) {
    uint8_t timeData[8];
    for (int i = 7; i >= 0; --i) {
        timeData[i] = timeStep & 0xFF;
        timeStep >>= 8;
    }

    unsigned int len = 0;
    unsigned char *hash = HMAC(EVP_sha1(), keyBytes.data(), keyBytes.size(), timeData, 8, nullptr, &len);

    if (!hash) return "";

    int offset = hash[len - 1] & 0x0F;
    int binary = ((hash[offset] & 0x7F) << 24) | ((hash[offset + 1] & 0xFF) << 16) | ((hash[offset + 2] & 0xFF) << 8) | (hash[offset + 3] & 0xFF);

    int otp = binary % 1000000;

    std::ostringstream ss;
    ss << std::setw(6) << std::setfill('0') << otp;
    return ss.str();
}
// ### <= Utils ###


std::expected<std::string, std::string> AuthSrv::hash_password(const std::string& password) {
    uint32_t t_cost = ConfigUtil::get_int("ARGON2_ITERATIONS", 3);
    uint32_t m_cost = ConfigUtil::get_int("ARGON2_MEMORY", 65536); // directly use KB
    uint32_t parallelism = ConfigUtil::get_int("ARGON2_PARALLELISM", 1);
    
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
    int result = argon2id_verify(hash.c_str(), password.c_str(), password.length());
    return result == ARGON2_OK;
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
    //return code.length() == 6 && !secret.empty();
    if (secret.empty() || code.length() != 6) return false;

    std::vector<uint8_t> keyBytes = AuthSrv::base32Decode(secret);
    int64_t currentStep = AuthSrv::getCurrentTimeStep();

    for (int i = -1; i <= 1; ++i) {
        if (generateCodeForStep(keyBytes, currentStep + i) == code) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Generates a random Base32 secret for TOTP.
 *
 * The secret is 32 characters long, consisting of upper-case letters A-Z and digits 2-7.
 *
 * @return The generated secret string.
 */
std::string AuthSrv::generate_totp_secret() {
    std::string secret;
    secret.reserve(32);

    std::random_device rd;
    std::uniform_int_distribution<int> dis(0, 31);

    for (int i = 0; i < 32; ++i) {
        secret.push_back(B32_CHARS[dis(rd)]);
    }

    return secret;
}

} // namespace drogon_auth
