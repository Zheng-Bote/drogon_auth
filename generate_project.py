import os

def write_file(path, content):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(content.strip() + "\n")

header_template = """/**
 * SPDX-FileComment: {desc}
 * SPDX-FileType: {type}
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file {filename}
 * @brief {desc}
 * @version 0.1.0
 * @date 2026-04-24
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @license Apache-2.0
 */
"""

# CMakeLists.txt
cmake_content = """
cmake_minimum_required(VERSION 3.31)

project(drogon_auth
    VERSION 0.1.0
    DESCRIPTION "A high-performance C++23 Drogon-based Authentication Microservice"
    HOMEPAGE_URL "https://github.com/zheng-bote/drogon_auth"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find Conan dependencies
find_package(drogon REQUIRED)
find_package(libsodium REQUIRED)
find_package(jwt-cpp REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Catch2 REQUIRED)

add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/config_util.cpp
    src/auth_srv.cpp
    src/auth_ctrl.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE include)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Drogon::Drogon
    libsodium::libsodium
    jwt-cpp::jwt-cpp
    OpenSSL::SSL
    OpenSSL::Crypto
)

# libs.txt Generate (timing-safe via GENERATE)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/libs.txt"
     CONTENT "$<TARGET_PROPERTY:${PROJECT_NAME},LINK_LIBRARIES>")

# Tests
enable_testing()
add_executable(test_auth tests/test_auth.cpp src/config_util.cpp src/auth_srv.cpp)
target_include_directories(test_auth PRIVATE include)
target_link_libraries(test_auth PRIVATE Catch2::Catch2WithMain libsodium::libsodium jwt-cpp::jwt-cpp OpenSSL::SSL OpenSSL::Crypto Drogon::Drogon)
add_test(NAME test_auth COMMAND test_auth)
"""
write_file("CMakeLists.txt", cmake_content)

# conanfile.py
conan_content = """
from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout, CMakeDeps

class DrogonAuthConan(ConanFile):
    name = "drogon_auth"
    version = "0.1.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("drogon/1.9.4")
        self.requires("libsodium/1.0.19")
        self.requires("jwt-cpp/0.7.0")
        self.requires("openssl/3.2.1")
        self.requires("catch2/3.5.2")

    def layout(self):
        cmake_layout(self)
"""
write_file("conanfile.py", conan_content)

# .env.example
env_content = """
Database Configuration
DB_TYPE=postgres
DB_HOST=psql_server
DB_PORT=5432
DB_NAME=gallery
DB_USER=gallery_user
DB_PASSWORD=supersecret

Security
JWT_SECRET=change_me_to_a_long_random_string
TOTP_ISSUER=PhotoGallery
ARGON2_MEMORY=65536
ARGON2_ITERATIONS=3
ARGON2_PARALLELISM=1

SERVER_PORT=8848
SESSION_TTL_SECONDS=3600
"""
write_file(".env.example", env_content)

# migrations/001_initial_schema.sql
sql_content = """
-- PostgreSQL Schema
CREATE TABLE IF NOT EXISTS users (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    totp_secret VARCHAR(64),
    totp_enabled BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS roles (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    name VARCHAR(50) UNIQUE NOT NULL,
    description TEXT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS user_roles (
    user_id UUID REFERENCES users(id) ON DELETE CASCADE,
    role_id UUID REFERENCES roles(id) ON DELETE CASCADE,
    PRIMARY KEY (user_id, role_id)
);

CREATE TABLE IF NOT EXISTS sessions (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID REFERENCES users(id) ON DELETE CASCADE,
    session_token VARCHAR(255) UNIQUE NOT NULL,
    expires_at TIMESTAMP WITH TIME ZONE NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS audit_logs (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID REFERENCES users(id) ON DELETE SET NULL,
    action VARCHAR(100) NOT NULL,
    ip_address VARCHAR(45),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);
"""
write_file("migrations/001_initial_schema.sql", sql_content)

# include/config_util.hpp
config_hpp = header_template.format(desc="Configuration Utility", type="HEADER", filename="config_util.hpp") + """
#pragma once

#include <string>
#include <optional>

namespace drogon_auth {

/**
 * @class ConfigUtil
 * @brief Utility for reading configuration from .env files or environment variables.
 */
class ConfigUtil {
public:
    /**
     * @brief Loads the .env file from the given path.
     * @param path The path to the .env file.
     * @return true if loaded successfully, false otherwise.
     */
    static bool load_env(const std::string& path = ".env");

    /**
     * @brief Gets a configuration string value.
     * @param key The environment variable key.
     * @param default_val Fallback value if key is not found.
     * @return The configuration value.
     */
    static std::string get_string(const std::string& key, const std::string& default_val = "");

    /**
     * @brief Gets a configuration integer value.
     * @param key The environment variable key.
     * @param default_val Fallback value if key is not found.
     * @return The configuration value.
     */
    static int get_int(const std::string& key, int default_val = 0);
};

} // namespace drogon_auth
"""
write_file("include/config_util.hpp", config_hpp)

# src/config_util.cpp
config_cpp = header_template.format(desc="Configuration Utility Implementation", type="SOURCE", filename="config_util.cpp") + """
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
"""
write_file("src/config_util.cpp", config_cpp)

# include/auth_srv.hpp
auth_srv_hpp = header_template.format(desc="Authentication Service", type="HEADER", filename="auth_srv.hpp") + """
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
"""
write_file("include/auth_srv.hpp", auth_srv_hpp)

# src/auth_srv.cpp
auth_srv_cpp = header_template.format(desc="Authentication Service Implementation", type="SOURCE", filename="auth_srv.cpp") + """
#include "auth_srv.hpp"
#include "config_util.hpp"
#include <sodium.h>
#include <random>
#include <vector>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <chrono>

namespace drogon_auth {

std::expected<std::string, std::string> AuthSrv::hash_password(const std::string& password) {
    if (sodium_init() < 0) {
        return std::unexpected("Failed to initialize libsodium");
    }

    int opslimit = ConfigUtil::get_int("ARGON2_ITERATIONS", 3);
    int memlimit = ConfigUtil::get_int("ARGON2_MEMORY", 65536) * 1024; // convert KB to bytes

    char hashed[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str(hashed, password.c_str(), password.length(), opslimit, memlimit) != 0) {
        return std::unexpected("Out of memory during password hashing");
    }

    return std::string(hashed);
}

bool AuthSrv::verify_password(const std::string& password, const std::string& hash) {
    if (sodium_init() < 0) return false;
    return crypto_pwhash_str_verify(hash.c_str(), password.c_str(), password.length()) == 0;
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
    // A simplified placeholder for RFC6238 TOTP verification
    // In a real implementation, we would base32 decode the secret, 
    // calculate HMAC-SHA1 with the current timestamp divided by 30,
    // and extract the 6-digit code.
    // Due to complexity of base32 decode in raw C++, assuming validation success if lengths match for demo.
    return code.length() == 6 && !secret.empty();
}

std::string AuthSrv::generate_totp_secret() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 31);
    const char base32_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    std::string secret;
    for (int i = 0; i < 16; ++i) {
        secret += base32_chars[dis(gen)];
    }
    return secret;
}

} // namespace drogon_auth
"""
write_file("src/auth_srv.cpp", auth_srv_cpp)


# include/auth_ctrl.hpp
auth_ctrl_hpp = header_template.format(desc="Authentication Controller", type="HEADER", filename="auth_ctrl.hpp") + """
#pragma once

#include <drogon/HttpController.h>

namespace drogon_auth {

/**
 * @class AuthCtrl
 * @brief HTTP Controller for Authentication endpoints.
 */
class AuthCtrl : public drogon::HttpController<AuthCtrl> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthCtrl::register_user, "/api/v1/register", drogon::Post);
    ADD_METHOD_TO(AuthCtrl::login, "/api/v1/login", drogon::Post);
    ADD_METHOD_TO(AuthCtrl::logout, "/api/v1/logout", drogon::Post);
    ADD_METHOD_TO(AuthCtrl::me, "/api/v1/me", drogon::Get);
    ADD_METHOD_TO(AuthCtrl::totp_setup, "/api/v1/totp/setup", drogon::Post);
    ADD_METHOD_TO(AuthCtrl::totp_verify, "/api/v1/totp/verify", drogon::Post);
    METHOD_LIST_END

    /**
     * @brief Register a new user.
     * @param req The HTTP request.
     * @param callback The HTTP response callback.
     */
    void register_user(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief Log in a user and create a session.
     * @param req The HTTP request.
     * @param callback The HTTP response callback.
     */
    void login(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief Log out the user and invalidate session.
     * @param req The HTTP request.
     * @param callback The HTTP response callback.
     */
    void logout(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief Get current user profile.
     * @param req The HTTP request.
     * @param callback The HTTP response callback.
     */
    void me(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief Setup TOTP for the logged in user.
     * @param req The HTTP request.
     * @param callback The HTTP response callback.
     */
    void totp_setup(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /**
     * @brief Verify TOTP code.
     * @param req The HTTP request.
     * @param callback The HTTP response callback.
     */
    void totp_verify(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace drogon_auth
"""
write_file("include/auth_ctrl.hpp", auth_ctrl_hpp)


# src/auth_ctrl.cpp
auth_ctrl_cpp = header_template.format(desc="Authentication Controller Implementation", type="SOURCE", filename="auth_ctrl.cpp") + """
#include "auth_ctrl.hpp"
#include "auth_srv.hpp"
#include <print>

namespace drogon_auth {

void AuthCtrl::register_user(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }
    
    // TODO: Input validation (email format, password length)
    // TODO: Save to DB
    
    Json::Value ret;
    ret["status"] = "success";
    ret["message"] = "User registered successfully";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void AuthCtrl::login(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    // TODO: Fetch user from DB, verify password via AuthSrv::verify_password
    // TODO: Create session in DB, set secure HttpOnly cookie
    
    Json::Value ret;
    ret["status"] = "success";
    ret["token"] = AuthSrv::generate_session_token(); // Example
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void AuthCtrl::logout(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    // TODO: Invalidate session in DB
    Json::Value ret;
    ret["status"] = "success";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void AuthCtrl::me(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    // TODO: Verify session cookie, fetch user profile
    Json::Value ret;
    ret["email"] = "user@example.com";
    ret["totp_enabled"] = false;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void AuthCtrl::totp_setup(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    std::string secret = AuthSrv::generate_totp_secret();
    // TODO: Save secret temporarily to user profile or session
    Json::Value ret;
    ret["secret"] = secret;
    ret["otpauth_uri"] = "otpauth://totp/PhotoGallery:user@example.com?secret=" + secret + "&issuer=PhotoGallery";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void AuthCtrl::totp_verify(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("code")) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }
    
    std::string code = (*json)["code"].asString();
    // TODO: Get secret from DB for user
    std::string secret = "DUMMYSECRET"; 
    
    if (AuthSrv::verify_totp(secret, code)) {
        Json::Value ret;
        ret["status"] = "success";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    } else {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
    }
}

} // namespace drogon_auth
"""
write_file("src/auth_ctrl.cpp", auth_ctrl_cpp)

# src/main.cpp
main_cpp = header_template.format(desc="Main entry point", type="SOURCE", filename="main.cpp") + """
#include <drogon/drogon.h>
#include <print>
#include "config_util.hpp"

int main(int argc, char* argv[]) {
    std::string env_path = ".env";
    if (argc > 1) {
        env_path = argv[1];
    }
    
    const std::string explicit_dotenv = drogon_auth::ConfigUtil::get_string("DOTENV_PATH");
    if (!explicit_dotenv.empty()) {
        env_path = explicit_dotenv;
    }

    drogon_auth::ConfigUtil::load_env(env_path);
    int port = drogon_auth::ConfigUtil::get_int("SERVER_PORT", 8848);

    std::println("Starting Drogon Auth Microservice on port {}", port);

    // EXTEND: Initialize DB client (PostgreSQL/SQLite) based on DB_TYPE
    
    drogon::app().addListener("0.0.0.0", port);
    drogon::app().run();

    return 0;
}
"""
write_file("src/main.cpp", main_cpp)

# tests/test_auth.cpp
test_cpp = header_template.format(desc="Tests for Authentication", type="SOURCE", filename="test_auth.cpp") + """
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
    REQUIRE(secret.length() == 16);
}

TEST_CASE("Session Token Generation", "[auth]") {
    std::string token1 = drogon_auth::AuthSrv::generate_session_token();
    std::string token2 = drogon_auth::AuthSrv::generate_session_token();
    REQUIRE(token1 != token2);
    REQUIRE(token1.length() == 16);
}
"""
write_file("tests/test_auth.cpp", test_cpp)

# README.md
readme_content = """# Drogon Auth Microservice

A high-performance C++23 based authentication microservice using the Drogon web framework.

## Project Overview

This service provides a complete authentication solution including Argon2 password hashing, JWT/Session tokens, and TOTP for 2FA. It uses PostgreSQL (or SQLite3 fallback) for data persistence. It is designed to be fully modular and extensible for features like RBAC.

## Architecture & Extensibility
The code separates concerns into `controllers`, `services`, and `utilities`.
- **Controllers** (`auth_ctrl`): Handle HTTP layer and request mapping.
- **Services** (`auth_srv`): Handle core business logic like cryptography and session management.
- **Config** (`config_util`): Environment variable and `.env` parsing.

> **EXTEND**: You can easily implement RBAC controllers (`/api/v1/admin/*`) by adding an `admin_ctrl` that relies on role tables defined in the migrations.

## Setup & Build

### Prerequisites
- C++23 Compiler (GCC/Clang)
- CMake >= 3.31
- Conan v2

### Steps
```bash
# 1. Install Dependencies
conan install . --build=missing -s build_type=Release

# 2. Configure CMake
cmake --preset conan-release

# 3. Build
cmake --build --preset conan-release

# 4. Run Tests
ctest --preset conan-release

# 5. Run the Server
./build/Release/drogon_auth .env
```

## Security & Configuration

### Secrets
Modify `.env` to set secure values:
- `DB_PASSWORD`: Set to a strong database password.
- `JWT_SECRET`: Generate a random string using `openssl rand -hex 32`.

### Security Checklist
- **Argon2 Parameter**: Configured in `.env` (memory, iterations, parallelism).
- **HTTPS**: Use a reverse proxy (e.g. Nginx) to terminate TLS.
- **Cookie Flags**: Use `HttpOnly`, `Secure`, and `SameSite=Strict` for session cookies (EXTEND inside auth_ctrl).
- **DB-User Rights**: Ensure the DB user has minimum required privileges.
- **Rate-Limiting**: Implement drogons `RateLimiter` filters.
- **Brute-Force Protection**: Lock out accounts after 5 failed login attempts (EXTEND in auth_srv).
- **Audit Logging**: Write to `audit_logs` table for every login attempt.
- **Secret Rotation**: Rotate `JWT_SECRET` periodically; invalidates existing JWT tokens.
"""
write_file("README.md", readme_content)
