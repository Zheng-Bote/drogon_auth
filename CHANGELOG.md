# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.7.0] - 2026-08-23

### Added
- **Session Protection**: Implemented strict Client Fingerprinting (IP & User-Agent). Invalid sessions trigger `requires_reauth`.
- **CSRF Tokens**: All state-mutating requests now require an `X-CSRF-TOKEN` header validated against the session cache.
- **In-Memory Cache**: Replaced DB lookups for sessions in `AuthMiddleware` with a high-performance, thread-safe `SessionCache` (`unordered_map` with `std::shared_mutex`).
- **Audit RingBuffer**: Created `LockFreeRingBuffer` via Atomics to store the most recent 100 authentication events in memory with zero-copy/lock-free performance.
- **Security Hardening**: Implemented Token-Bucket `RateLimiter` to protect login, TOTP, and password reset endpoints from brute-force attacks.
- **Device Awareness**: Trigger `new_device_login` audit events when a user logs in from an unknown IP or device.
- **Cookie Security**: Enforced `Secure` and `SameSite=Strict` flags for `JSESSIONID` cookies.
- **TLS Configuration**: Native support for binding to TLS via Drogon directly (`SERVER_SSL_CERT` and `SERVER_SSL_KEY`).
- **Comprehensive Documentation**: Restructured `docs/` and provided fully consolidated, English-language Architecture, API, Security, Developer, Operations, and Troubleshooting guides.

### Changed
- **CMake Target**: Updated minimum CMake version to 3.31 and switched default build instructions to Release (`conan-release`).
- **System Endpoints**: `/api/auth/system/sys-info` now retrieves and returns the backend `database_version` dynamically.
- **Session Refresh**: Added `/api/auth/v1/refresh` endpoint to manually extend active sessions for 24h.
- **Background Workers**: Hooked coroutine tasks into Drogon's `getLoop()->runEvery()` to asynchronously clean up expired database sessions (every 5min) and sync the In-Memory Cache (every 1min).

## [0.6.0] - 2026-05-03

### Added
- **gRPC API**: Integrated a native gRPC server running alongside Drogon.
- **Session Verification (gRPC)**: New `GetUserStatus` RPC method allowing other microservices to verify sessions and retrieve user roles and active status.
- **Protobuf Support**: Added `proto/auth_service.proto` and build-time code generation.
- **gRPC Configuration**: Added `GRPC_PORT` to environment configuration.

## [0.5.0] - 2026-05-02

### Added
- **Role CRUD**: Implemented full Create, Read, Update, and Delete functionality for system roles in `AdminCtrl`.
- **Enhanced Security Metadata**: 
    - `/api/auth/v1/me` and `/api/auth/v1/profile` now include 2FA status and last login/password change timestamps.
    - Added `must_pwd_change` support to user management.
- **Audit Summary**: New endpoint `/api/auth/admin/v1/audit/summary` providing activity counts for the last 7 days.
- **Forced Password Change**: New endpoint `/api/auth/v1/password/change-forced` to handle mandatory password updates before session creation.

### Changed
- **Error Handling**: Improved `AdminCtrl` to catch database constraint violations and return `409 Conflict` with descriptive JSON messages (e.g., for duplicate loginnames or role names).
- **Login Flow**: Updated authentication logic to detect `must_pwd_change` flag and return a specific status to the frontend.

## [0.4.0] - 2026-05-01
### Added
- **Admin UI**: Full-featured web-based administration frontend built with Vue 3 and Quasar.
- **Admin Management**: New `AdminCtrl` in the backend for CRUD operations on users and roles.
- **Two-Factor Authentication (2FA)**: Enforcement of TOTP during the login process (two-step login).
- **QR Code Support**: Integration of `qrcode.vue` for easy 2FA setup in the frontend.
- **Profile Management**: Endpoints and UI for managing personal data, communication channels, and security settings.
- **Audit Logging**: Enhanced audit logging via `AuditLogPlugin` for login (MFA included), logout, password changes, and 2FA activations.
- **Explicit Transactions**: Added manual `COMMIT` calls to all database transactions in controllers and seeder for PostgreSQL compatibility.

### Changed
- Switched Argon2id implementation from `argon2` C library to `libsodium` (libsodium/1.0.21).
- Refactored `PasswordUtils` to use `crypto_pwhash_str` and `crypto_pwhash_str_verify` from libsodium.
- Consolidated password hashing logic by delegating `AuthSrv::hash_password` to `PasswordUtils`.
- Standardized API error responses to return consistent JSON objects (401, 403, 404, 500).
- Unified session cookie name to `JSESSIONID` across all endpoints.
- Updated `Seeder` to automatically create a user profile when generating the initial admin account.

### Fixed
- Fixed PostgreSQL binary format errors for `INET` and `BOOLEAN` types in SQL queries using explicit `CAST` and C++ `bool`.
- Fixed data loss issue in profile updates by ensuring all profile data (including communications) is sent in a single request.
- Fixed 404 error on profile page caused by missing initial profile records.

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
