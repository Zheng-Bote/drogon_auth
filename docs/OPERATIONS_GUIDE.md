# Drogon Auth - Operations Guide

## 1. Prerequisites
- **Linux** Environment (Ubuntu/Debian recommended).
- **PostgreSQL >= 18** (or SQLite3 for dev/testing).
- **CMake >= 3.31** and **Conan**.

## 2. Environment Configuration
The service relies on environment variables (which can be loaded from a `.env` file via `ConfigUtil`).

| Variable | Description | Default |
|----------|-------------|---------|
| `DB_TYPE` | Database driver (`postgres` or `sqlite3`) | `postgres` |
| `DB_HOST` | PostgreSQL Host | `127.0.0.1` |
| `DB_NAME` | PostgreSQL Database Name | `drogon_auth` |
| `SERVER_PORT` | Port for the HTTP Service | `8080` |
| `SERVER_THREADS` | Number of I/O worker threads | `4` |
| `SERVER_SSL_CERT` | Path to TLS Certificate | *(Empty)* |
| `SERVER_SSL_KEY` | Path to TLS Private Key | *(Empty)* |
| `JWT_SECRET` | 32+ char secret for JWT generation | *(Required)* |

## 3. Build & Deployment
### Compiling
```bash
conan install . --build=missing -s build_type=Release
cmake --preset conan-release
cmake --build --preset conan-release -j$(nproc)
```

### Running as a Systemd Service
Create `/etc/systemd/system/drogon-auth.service`:
```ini
[Unit]
Description=Drogon Auth Microservice
After=network.target postgresql.service

[Service]
Type=simple
User=drogon
WorkingDirectory=/opt/drogon_auth
ExecStart=/opt/drogon_auth/build/Release/drogon_auth .env
Restart=always

[Install]
WantedBy=multi-user.target
```

## 4. Monitoring & Observability
- Configure Prometheus to scrape `http://<server-ip>:8080/api/auth/system/metrics`.
- The metrics expose `drogon_auth_active_sessions`, `drogon_auth_failed_logins`, and API latencies.
