# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.4.0] - 2026-04-29
### Changed
- Switched Argon2id implementation from `argon2` C library to `libsodium` (libsodium/1.0.21).
- Refactored `PasswordUtils` to use `crypto_pwhash_str` and `crypto_pwhash_str_verify` from libsodium.
- Consolidated password hashing logic by delegating `AuthSrv::hash_password` to `PasswordUtils`.

## [0.3.0] - 2026-04-28
### Added
- Implemented `AuthMiddleware` (modern Drogon Middleware) to replace the non-functioning `LoginFilter`.
- Added `Seeder` utility to ensure at least one admin account exists on startup.
- Integrated `registerBeginningAdvice` for reliable startup logging into log files.
- Integrated `AuthMiddleware` into `AuthCtrl` and `SystemCtrl`.

### Changed
- Refactored `AuthCtrl` to use session-based authentication flags.
- Switched to standard `JSESSIONID` for session management.
- Updated `main.cpp` to use modern Drogon initialization patterns.

### Removed
- Obsolete `LoginFilter` implementation.

## [0.2.0] - 2026-04-26
### Added
- TOTP logic for Google/Microsoft Authenticator
- optional configuration options with Drogon config
- static HTML output on URL `/` (Drogon config)

### Changed  
- URL notation to `/<api>/<service>/<version>/<category>`
- sys-info with DB-Version
- extending health-check

## [0.1.0] - 2026-04-24
### Added
- Initial release of Drogon Auth Microservice.
- Implementation of C++23 based HTTP controllers and services.
- Argon2id explicit password hashing via the `argon2` C library.
- Secure Session handling with HttpOnly cookies.
- TOTP 2FA setup and verification endpoints.
- PostgreSQL and SQLite3 database configuration via `.env`.
- Password change and reset functionality using C++20 Coroutines.
- Complete API documentation and architecture diagrams.
