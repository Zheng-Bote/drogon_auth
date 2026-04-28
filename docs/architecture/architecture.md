# Architecture Description

## Overview
The Drogon Auth Microservice is built on the **Drogon Framework** (C++23). It follows a layered architecture to ensure scalability, security, and maintainability.

## Layers

### 1. Presentation Layer (Controllers)
- **AuthCtrl**: Handles registration, login, logout, and 2FA.
- **SystemCtrl**: Provides health checks and system information.
- Utilizes C++20 Coroutines (`drogon::Task`) for non-blocking I/O.

### 2. Security Layer (Middleware)
- **AuthMiddleware**: Intercepts requests to protected resources. It validates the session status (`authenticated` flag) and ensures only authorized users proceed.
- Replaces the legacy `LoginFilter` with a more robust, stateful approach.

### 3. Business Logic Layer (Services)
- **AuthSrv**: Contains core logic for password hashing (Argon2id), TOTP generation/verification, and token management.
- **Seeder**: Ensures the system is in a consistent state on startup (e.g., creating an initial admin account).

### 4. Data Access Layer (ORM / DB)
- Uses Drogon's asynchronous ORM.
- Supports PostgreSQL (Production) and SQLite3 (Development).

## Security Patterns
- **Session-Based Authentication**: Uses `HttpOnly` and `SameSite=Strict` cookies.
- **Argon2id**: Industry-standard password hashing.
- **RBAC**: Role-Based Access Control via database-defined roles.
- **Audit Trail**: Every sensitive action is logged in the `audit_logs` table.

## Initialization Flow
1. **main.cpp**: Loads environment (`.env`) and configuration (`config.json`).
2. **Framework Initialization**: Drogon sets up the event loop and logging.
3. **Beginning Advice**: 
    - Logs startup parameters.
    - Runs the `Seeder` to ensure an admin exists.
4. **Listener**: Starts listening for HTTP requests.
