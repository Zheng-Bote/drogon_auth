# Database Description

The Drogon Auth Microservice uses a relational database (PostgreSQL or SQLite3) to manage users, sessions, and security-related data.

## Tables

### 1. `users`
The core table for user accounts.
- `password_hash`: Stores the Argon2id encoded string.
- `is_active`: Global flag to enable/disable account.

### 2. `roles` & `user_roles`
Implements Role-Based Access Control (RBAC).
- Default roles: `admin`, `user`.

### 3. `sessions`
Server-side session storage. 
- While the app uses `JSESSIONID` for active requests, this table provides persistence across server restarts and allows for session invalidation (e.g., from a different device).

### 4. `user_profiles`
Stores non-authentication related user data (names, preferences).

### 5. `user_communications`
An extensible table for contact methods. Supports multiple channels like email, phone, or messaging IDs.

### 6. `totp_secrets`
Stores secrets for Two-Factor Authentication (2FA). Secrets should be treated as highly sensitive.

### 7. `audit_logs` & `login_attempts`
Used for security monitoring and brute-force protection.
- `login_attempts` tracks both successful and failed logins with IP address.

### 8. `password_resets`
Manages tokens for the password recovery flow.

## Database Support
The application supports:
- **PostgreSQL**: Recommended for production (utilizes `pgcrypto` and `JSONB`).
- **SQLite3**: Supported for local development or lightweight deployments.
