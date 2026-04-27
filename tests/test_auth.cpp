/**
 * SPDX-FileComment: Tests for Authentication
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file test_auth.cpp
 * @brief Tests for Authentication
 * @version 0.1.0
 * @date 2026-04-24
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
#include <catch2/catch_test_macros.hpp>
#include "auth_srv.hpp"

TEST_CASE("Password Hashing", "[auth]") {
    std::string pwd = "my_secure_password";
    auto hash_result = drogon_auth::AuthSrv::hash_password(pwd);
    REQUIRE(hash_result.has_value());
    
    bool verified = drogon_auth::AuthSrv::verify_password(pwd, hash_result.value());
    REQUIRE(verified == true);
    
    bool failed = drogon_auth::AuthSrv::verify_password("wrong_pwd", hash_result.value());
    REQUIRE(failed == false);
}

TEST_CASE("TOTP Secret Generation", "[auth]") {
    std::string secret = drogon_auth::AuthSrv::generate_totp_secret();
    REQUIRE(secret.length() == 32);
}

TEST_CASE("Session Token Generation", "[auth]") {
    std::string token1 = drogon_auth::AuthSrv::generate_session_token();
    std::string token2 = drogon_auth::AuthSrv::generate_session_token();
    REQUIRE(token1 != token2);
    REQUIRE(token1.length() == 32);
}
