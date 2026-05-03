# Database Description

The Drogon Auth Microservice uses a relational database (PostgreSQL or SQLite3) to manage users, sessions, and security-related data.

## Entity Relationship Overview

```mermaid
erDiagram
    USERS ||--o| USER_PROFILES : "has"
    USERS ||--o{ USER_ROLES : "assigned"
    USERS ||--o{ SESSIONS : "owns"
    USERS ||--o{ LOGIN_ATTEMPTS : "records"
    USERS ||--o{ USER_COMMUNICATIONS : "uses"
    USERS ||--o| TOTP_SECRETS : "protected by"
    ROLES ||--o{ USER_ROLES : "links to"
    USERS ||--o{ AUDIT_LOGS : "logs actions"
```

## Tables

### 1. `users`

The core table for user accounts.

- `password_hash`: Stores the Argon2id encoded string (libsodium format).
- `is_active`: Global flag to enable/disable account.
- `must_pwd_change`: Boolean flag for forcing password rotation.

### 2. `roles` & `user_roles`

Implements Role-Based Access Control (RBAC).

- `roles`: `admin`, `user`.

### 3. `sessions`

Server-side session storage.

- `ip_address`: Uses `INET` type (PostgreSQL).
- `expires_at`: Managed via trantor date-time.

### 4. `user_profiles`

Stores personal data.

- `first_name`, `last_name`, `preferred_language`, `timezone`.

### 5. `user_communications`

Contact methods for notifications.

- `channel`: Enum (`email`, `pushover`, `telegram`, `whatsapp`).

### 6. `totp_secrets`

Sensitive secrets for Two-Factor Authentication.

- If a record exists here for a user, MFA is enforced during login.

### 7. `audit_logs` & `login_attempts`

Security monitoring.

- `audit_logs`: Detailed JSON data for security-relevant events.
- `login_attempts`: Specific tracking of auth success/failure with IP and user agent.

### 8. `password_resets`

Tokens for the secure password recovery flow.

## Implementation Notes

- **PostgreSQL**: Uses `pgcrypto` for UUID generation and `INET` for network addresses.
- **SQLite3**: Compatibility layer using standard text/integer fields where specialized types aren't available.
- **Transactions**: All operations involving multiple tables or security-critical data use explicit transactions with manual `COMMIT`.

## Entity Relationship Diagram (ERD)

This diagram describes the database schema used by the Drogon Auth Microservice.

```mermaid
erDiagram
    users ||--o| user_profiles : "has"
    users ||--o{ user_communications : "uses"
    users ||--o| totp_secrets : "secures"
    users ||--o{ user_roles : "assigned"
    users ||--o{ sessions : "active"
    users ||--o{ audit_logs : "logs"
    users ||--o{ login_attempts : "attempts"
    users ||--o{ password_resets : "requests"
    roles ||--o{ user_roles : "contains"

    users {
        uuid id PK
        string loginname UK
        string email UK
        string password_hash
        boolean is_active
        boolean must_pwd_change
        timestamp created_at
        timestamp updated_at
    }

    user_profiles {
        uuid id PK
        uuid user_id FK
        string first_name
        string last_name
        string preferred_language
        string locale
        string timezone
        jsonb attributes
        timestamp created_at
        timestamp updated_at
    }

    user_communications {
        uuid id PK
        uuid user_id FK
        enum channel
        string address
        boolean is_active
        boolean verified
        jsonb metadata
        timestamp created_at
        timestamp updated_at
    }

    roles {
        uuid id PK
        string name UK
        string description
        timestamp created_at
    }

    user_roles {
        uuid user_id PK, FK
        uuid role_id PK, FK
        timestamp assigned_at
    }

    sessions {
        uuid id PK
        uuid user_id FK
        string session_token UK
        timestamp expires_at
        timestamp created_at
        timestamp last_accessed_at
        inet ip_address
        text user_agent
        jsonb data
    }

    totp_secrets {
        uuid id PK
        uuid user_id FK
        text secret
        string issuer
        timestamp created_at
        timestamp updated_at
    }

    audit_logs {
        uuid id PK
        uuid user_id FK
        string action
        string ip_address
        jsonb details
        timestamp created_at
    }

    login_attempts {
        uuid id PK
        uuid user_id FK
        string loginname
        inet ip_address
        boolean success
        timestamp created_at
    }

    password_resets {
        uuid id PK
        uuid user_id FK
        string token UK
        timestamp expires_at
        boolean used
        timestamp created_at
    }
```
