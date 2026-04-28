-- migrations/001_create_auth_schema.sql
-- Idempotente Migration: Auth / Users / Sessions / RBAC / Audit
-- PostgreSQL, benötigt extension pgcrypto for gen_random_uuid()

-- Extensions
CREATE EXTENSION IF NOT EXISTS pgcrypto;

-- Database: drogon_auth

-- DROP DATABASE IF EXISTS drogon_auth;

CREATE DATABASE drogon_auth
    WITH
    OWNER = postgres
    ENCODING = 'UTF8'
    LC_COLLATE = 'en_US.utf8'
    LC_CTYPE = 'en_US.utf8'
    LOCALE_PROVIDER = 'libc'
    TABLESPACE = pg_default
    CONNECTION LIMIT = -1
    IS_TEMPLATE = False;

GRANT TEMPORARY, CONNECT ON DATABASE drogon_auth TO PUBLIC;

GRANT TEMPORARY ON DATABASE drogon_auth TO drogon_auth_user;

GRANT ALL ON DATABASE drogon_auth TO postgres;

-- ------------------------------------------------------------------
-- Users table (auth core) - abgeglichen mit deinem Referenzschema
-- ------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS users (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    loginname VARCHAR(100) UNIQUE NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL, -- store full Argon2id encoded string
    is_active BOOLEAN NOT NULL DEFAULT TRUE,
    must_pwd_change BOOLEAN NOT NULL DEFAULT FALSE,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_users_loginname ON users(loginname);
CREATE UNIQUE INDEX IF NOT EXISTS idx_users_email ON users(email);

CREATE TABLE IF NOT EXISTS password_resets (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    token VARCHAR(255) UNIQUE NOT NULL,
    expires_at TIMESTAMP WITH TIME ZONE NOT NULL,
    used BOOLEAN NOT NULL DEFAULT FALSE,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_password_resets_token ON password_resets(token);
CREATE INDEX IF NOT EXISTS idx_password_resets_user_id ON password_resets(user_id);

-- ------------------------------------------------------------------
-- Profiles: personal data separate from auth
-- ------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS user_profiles (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    first_name VARCHAR(100),
    last_name VARCHAR(100),
    preferred_language VARCHAR(10) DEFAULT 'en',
    locale VARCHAR(10),
    timezone VARCHAR(64),
    attributes JSONB DEFAULT '{}'::jsonb,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (user_id)
);

CREATE INDEX IF NOT EXISTS idx_user_profiles_user_id ON user_profiles(user_id);

-- ------------------------------------------------------------------
-- Communications: extensible contact methods (email, pushover, telegram, ...)
-- ------------------------------------------------------------------
DO $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'communication_channel') THEN
        CREATE TYPE communication_channel AS ENUM (
            'email',
            'pushover',
            'telegram',
            'wechat',
            'whatsapp',
            'sms',
            'push_generic',
            'other'
        );
    END IF;
END$$;

CREATE TABLE IF NOT EXISTS user_communications (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    channel communication_channel NOT NULL,
    address TEXT NOT NULL, -- e.g. email address, pushover user key, telegram chat id, phone number
    is_active BOOLEAN NOT NULL DEFAULT TRUE,
    verified BOOLEAN NOT NULL DEFAULT FALSE,
    metadata JSONB DEFAULT '{}'::jsonb, -- provider specific data (e.g. {"chat_id":"12345","device":"iPhone"})
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_user_communications_user_id ON user_communications(user_id);
CREATE INDEX IF NOT EXISTS idx_user_communications_channel ON user_communications(channel);

-- ------------------------------------------------------------------
-- TOTP secrets (separate table)
-- ------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS totp_secrets (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    secret TEXT NOT NULL, -- base32 or encrypted blob; treat as sensitive
    issuer VARCHAR(128),
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (user_id)
);

CREATE INDEX IF NOT EXISTS idx_totp_user_id ON totp_secrets(user_id);

-- ------------------------------------------------------------------
-- Roles and user_roles (RBAC) - abgeglichen mit deinem Referenzschema
-- ------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS roles (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    name VARCHAR(50) UNIQUE NOT NULL,
    description TEXT,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS user_roles (
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    role_id UUID NOT NULL REFERENCES roles(id) ON DELETE CASCADE,
    assigned_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (user_id, role_id)
);

CREATE INDEX IF NOT EXISTS idx_user_roles_user_id ON user_roles(user_id);
CREATE INDEX IF NOT EXISTS idx_user_roles_role_id ON user_roles(role_id);

-- ------------------------------------------------------------------
-- Sessions (server-side) - abgeglichen mit deinem Referenzschema
-- ------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS sessions (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID REFERENCES users(id) ON DELETE CASCADE,
    session_token VARCHAR(255) UNIQUE NOT NULL,
    expires_at TIMESTAMP WITH TIME ZONE NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    last_accessed_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    ip_address INET,
    user_agent TEXT,
    data JSONB DEFAULT '{}'::jsonb
);

CREATE INDEX IF NOT EXISTS idx_sessions_user_id ON sessions(user_id);
CREATE INDEX IF NOT EXISTS idx_sessions_expires_at ON sessions(expires_at);

-- ------------------------------------------------------------------
-- Audit logs - abgeglichen mit deinem Referenzschema
-- ------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS audit_logs (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID REFERENCES users(id) ON DELETE SET NULL,
    action VARCHAR(100) NOT NULL,
    ip_address VARCHAR(45),
    details JSONB DEFAULT '{}'::jsonb,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_audit_logs_user_id ON audit_logs(user_id);

-- ------------------------------------------------------------------
-- Optional: login_attempts table for brute-force protection (recommended)
-- ------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS login_attempts (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID REFERENCES users(id) ON DELETE CASCADE,
    loginname VARCHAR(100),
    ip_address INET,
    success BOOLEAN NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_login_attempts_user_id ON login_attempts(user_id);
CREATE INDEX IF NOT EXISTS idx_login_attempts_loginname ON login_attempts(loginname);

-- ------------------------------------------------------------------
-- Trigger function to update updated_at on update
-- ------------------------------------------------------------------
CREATE OR REPLACE FUNCTION trigger_set_timestamp()
RETURNS TRIGGER AS $$
BEGIN
   NEW.updated_at = CURRENT_TIMESTAMP;
   RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Attach triggers to tables that have updated_at
DO $$
BEGIN
   IF NOT EXISTS (SELECT 1 FROM pg_trigger WHERE tgname = 'set_timestamp_users') THEN
       CREATE TRIGGER set_timestamp_users
       BEFORE UPDATE ON users
       FOR EACH ROW
       EXECUTE PROCEDURE trigger_set_timestamp();
   END IF;

   IF NOT EXISTS (SELECT 1 FROM pg_trigger WHERE tgname = 'set_timestamp_profiles') THEN
       CREATE TRIGGER set_timestamp_profiles
       BEFORE UPDATE ON user_profiles
       FOR EACH ROW
       EXECUTE PROCEDURE trigger_set_timestamp();
   END IF;

   IF NOT EXISTS (SELECT 1 FROM pg_trigger WHERE tgname = 'set_timestamp_communications') THEN
       CREATE TRIGGER set_timestamp_communications
       BEFORE UPDATE ON user_communications
       FOR EACH ROW
       EXECUTE PROCEDURE trigger_set_timestamp();
   END IF;

   IF NOT EXISTS (SELECT 1 FROM pg_trigger WHERE tgname = 'set_timestamp_totp') THEN
       CREATE TRIGGER set_timestamp_totp
       BEFORE UPDATE ON totp_secrets
       FOR EACH ROW
       EXECUTE PROCEDURE trigger_set_timestamp();
   END IF;
END$$;

-- ------------------------------------------------------------------
-- Seed default roles (idempotent)
-- ------------------------------------------------------------------
INSERT INTO roles (id, name, description)
SELECT gen_random_uuid(), 'user', 'Default role for authenticated users'
WHERE NOT EXISTS (SELECT 1 FROM roles WHERE name = 'user');

INSERT INTO roles (id, name, description)
SELECT gen_random_uuid(), 'admin', 'Administrator role'
WHERE NOT EXISTS (SELECT 1 FROM roles WHERE name = 'admin');

-- ------------------------------------------------------------------
-- Example: ensure at least one admin user placeholder (optional)
-- (Uncomment and adapt if you want to seed a user; password_hash must be generated by app)
-- ------------------------------------------------------------------
-- INSERT INTO users (id, loginname, email, password_hash, is_active, must_pwd_change)
-- SELECT gen_random_uuid(), 'admin', 'admin@example.com', '$argon2id$...', TRUE, FALSE
-- WHERE NOT EXISTS (SELECT 1 FROM users WHERE loginname = 'admin');

