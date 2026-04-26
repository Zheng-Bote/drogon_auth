# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
