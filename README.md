# Drogon Auth Microservice

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

## Kommnentar

### Sicherheitskritische Stellen & Secrets (Empfehlungen)

- **DB_PASSWORD**: (.env) Sollte ein starkes Passwort für die DB-Zugriffskontrolle sein.
- **JWT_SECRET**: (.env) MUSS zwingend ein langer, sicherer Zufallsstring sein, z. B. generiert über openssl rand -hex 32.
- **auth_ctrl.cpp:login**: Hier muss zwingend ein Session-Cookie gesetzt werden. Setze dort die Flags HttpOnly, Secure (falls unter HTTPS) und SameSite=Strict.
- **auth_srv.cpp:verify_totp**: In einer produktiven Umgebung muss das RFC 6238-konforme Verfahren implementiert sein (derzeit als Mock realisiert). Die HMAC-Generierung über OpenSSL-Libraries ist der sicherste Weg hierfür.

### Security-Checklist

- **Argon2 Parameter**: Werte für Memory, Iterations und Parallelism werden zur Laufzeit konfiguriert (aus .env oder mit sicheren Defaults versehen).
- **HTTPS**: Drogon oder ein Reverse-Proxy (wie Nginx/HAProxy) erzwingt TLS-Absicherung der Endpunkte.
- **Cookie Flags**: Cookies sind zwingend mit HttpOnly, Secure (bei HTTPS) und SameSite=Strict zu definieren.
- **DB-User Rechte**: Die Anwendung läuft auf einem abgetrennten PostgreSQL-User, der nur DML-Operationen auf sein Schema anwenden darf.
- **Rate-Limiting**: Simple In-Memory Limits (z. B. Drogon Plugin RateLimiter) zum Schutz vor automatisierten Attacken einfügen.
- **Brute-Force Schutz**: Kontosperre nach x-fehlgeschlagenen Login-Versuchen wird per Auth-Service implementiert.
- **Audit Logging**: Die Tabelle audit_logs ist vorbereitet, um kritische Account-Aktionen (Anmeldung, Logout, 2FA-Änderung) nachvollziehbar zu protokollieren.
- **Secret Rotation**: Ablauf-Routinen für den periodischen Wechsel des JWT-Signing-Keys (bzw. Neuvergabe von Session-Tokens) vorbereiten.
