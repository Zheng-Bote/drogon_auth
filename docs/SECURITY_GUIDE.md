# Drogon Auth - Security & Hardening Guide

## 1. Authentication & Cryptography
- **Password Hashing**: We use Argon2id via `libsodium` (`crypto_pwhash_str`). This provides memory-hard hashing and constant-time string comparisons to mitigate timing attacks.
- **MFA / TOTP**: Compliant with RFC 6238. When MFA is enabled, login requests return a `mfa_required` state, demanding a subsequent validation through the `/login_totp` endpoint.
- **JWT**: JSON Web Tokens are used as internal session identifiers and for cross-service authentication (signed via `jwt-cpp`).

## 2. Session Protection
- **Secure Cookies**: The `JSESSIONID` cookie is enforced with `HttpOnly`, `Secure`, and `SameSite=Strict` flags to prevent XSS and cross-site tracking.
- **Client Fingerprinting**: IP addresses and `User-Agent` headers are bound to the session. If the user's IP or device changes, the session is aggressively invalidated, requiring re-authentication.
- **Device Awareness**: Logins from new devices/IPs automatically generate `new_device_login` audit events.
- **CSRF Protection**: All state-mutating HTTP methods (`POST`, `PUT`, `DELETE`, `PATCH`) require a valid `X-CSRF-TOKEN` header.

## 3. Infrastructure & Defense
- **SQL Injection Prevention**: The application exclusively uses PostgreSQL parameterized queries (`$1`, `$2`) via Drogon's `DbClient`.
- **IP Rate Limiting**: An in-memory, token-bucket `RateLimiter` restricts brute-force attempts (e.g., max 5 requests per minute for logins).
- **TLS**: Drogon is configured to require HTTPS communication by natively injecting `SERVER_SSL_CERT` and `SERVER_SSL_KEY` at startup.

## 4. Threat Mitigation Matrix (STRIDE)

| Threat | Mitigation Strategy |
|--------|---------------------|
| **Spoofing** | Argon2id passwords, JWT tokens, Strict Session Validation |
| **Tampering** | Parameterized SQL, JWT Signatures, CSRF Tokens |
| **Repudiation** | Lock-Free Audit RingBuffer, Persistent Audit Log Table |
| **Information Disclosure** | TLS/HTTPS enforcement, No sensitive data in logs, Zero-knowledge password hashes |
| **Denial of Service** | Coroutines (non-blocking I/O), IP Rate Limiting, In-Memory Session Cache |
| **Elevation of Privilege** | Granular RBAC Engine, Separation of roles and permissions in PostgreSQL views |
