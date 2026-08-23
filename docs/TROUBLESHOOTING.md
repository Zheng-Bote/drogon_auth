# Drogon Auth - Troubleshooting Guide

## 1. Database Connection Issues
**Symptom**: `healthCheck` returns `database: disconnected` or the application fails to start.
**Action**:
- Verify `DB_HOST`, `DB_USER`, and `DB_PASSWORD` in your `.env` file.
- If using PostgreSQL, ensure the user has permissions to the `drogon_auth` database.
- Check if PostgreSQL requires GSSAPI. Our CMake build automatically appends GSSAPI flags (`-lgssapi_krb5`) to `libpq`, but you may need to install `libkrb5-dev` on your host machine.

## 2. Session / Re-Authentication Loops
**Symptom**: Users are constantly logged out and receive `requires_reauth: true`.
**Action**:
- The `AuthMiddleware` verifies IP addresses and User-Agents. If your IAM service is behind a Reverse Proxy (like NGINX or HAProxy), ensure the proxy forwards the original IP.
- Add `proxy_set_header X-Real-IP $remote_addr;` to NGINX.

## 3. High CPU Usage / Latency
**Symptom**: The `/system/metrics` endpoint shows high latencies (`drogon_auth_api_latency_ms_sum`).
**Action**:
- Verify that `SERVER_THREADS` matches the number of available CPU cores.
- Ensure the database is indexing correctly. Specifically, check the `sessions` table.
- Verify the In-Memory Session Cache is operating correctly (check logs for "Background-Worker: Synced Session-Cache from DB").

## 4. CMake / Conan Build Failures
**Symptom**: `No version information available (required by cmake)` related to `libcurl`.
**Action**:
- This is a non-fatal warning often caused by Conan's localized `libcurl` conflicting with the system's `cmake`. It can be safely ignored as long as the build completes.

## 5. Coroutine / C++23 Compilation Errors
**Symptom**: `await expressions are not permitted in handlers`.
**Action**:
- `co_await` cannot be used inside `catch(...)` blocks. Move the logic outside or handle it gracefully using standard C++ control flows.
