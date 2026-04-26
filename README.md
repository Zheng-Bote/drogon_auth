# Drogon Auth Microservice

A high-performance C++23 based authentication microservice using the Drogon web framework.

## Project Overview

This service provides a complete authentication solution including Argon2id password hashing, JWT/Session tokens, and TOTP for 2FA. It uses PostgreSQL (or SQLite3 fallback) for data persistence. It is designed to be fully modular and extensible for features like RBAC.

## Architecture & Extensibility

The code separates concerns into `controllers`, `services`, and `utilities`.

- **Controllers** (`auth_ctrl`): Handle HTTP layer and request mapping.
- **Services** (`auth_srv`): Handle core business logic like cryptography and session management.
- **Config** (`config_util`): Environment variable and `.env` parsing.

> **EXTEND**: You can easily implement RBAC controllers (`/api/auth/v1/admin/*`) by adding an `admin_ctrl` that relies on role tables defined in the migrations.

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

## auth Documentation

The following table describes the implemented endpoints:

| Endpoint                         | Method | JSON Body Parameters                               | Description                                                                             |
| -------------------------------- | ------ | -------------------------------------------------- | --------------------------------------------------------------------------------------- |
| `/api/api/auth/v1/register`               | `POST` | `loginname` (str), `email` (str), `password` (str) | Registers a new user. Creates entries in users, user_profiles, and user_communications. |
| `/api/auth/v1/login`                  | `POST` | `loginname` OR `email` (str), `password` (str)     | Authenticates a user. Sets secure HttpOnly cookie and logs attempt.                     |
| `/api/auth/v1/logout`                 | `POST` | _None_                                             | Invalidates the active session and clears the cookie.                                   |
| `/api/auth/v1/me`                     | `GET`  | _None_                                             | Returns the current user's profile information.                                         |
| `/api/auth/v1/totp/setup`             | `POST` | _None_                                             | Generates a new TOTP secret for the user and returns the `otpauth://` URI.              |
| `/api/auth/v1/totp/verify`            | `POST` | `code` (str)                                       | Verifies a 6-digit TOTP code against the user's stored secret.                          |
| `/api/auth/v1/password/change`        | `POST` | `old_password` (str), `new_password` (str)         | Allows an authenticated user to change their password.                                  |
| `/api/auth/v1/password/reset-request` | `POST` | `email` (str)                                      | Generates a password reset token for the specified email.                               |
| `/api/auth/v1/password/reset-confirm` | `POST` | `token` (str), `new_password` (str)                | Resets the password using a valid reset token.                                          |
| `/api/auth/system/getVersion`         | `GET`  | _None_                                             | Returns the current version of the microservice.                                        |
| `/api/auth/system/health-check`       | `GET`  | _None_                                             | Checks Drogon server status and Database connection.                                    |
| `/api/auth/system/check-update`       | `GET`  | _None_                                             | Checks GitHub for a newer version of the microservice.                                  |
| `/api/auth/system/sys-info`           | `GET`  | _None_                                             | Returns detailed project metadata and compiler information.                             |

## Architecture

For a detailed architectural overview using C4 Model, please view the [Component Diagram](docs/architecture/component_diagram.md).
