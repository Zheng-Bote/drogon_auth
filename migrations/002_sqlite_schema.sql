-- migrations/002_sqlite_schema.sql
-- Adds missing tables (permissions, role_permissions, password_history)
-- Implements RBAC Views

-- ------------------------------------------------------------------
-- 1. Missing Tables: Permissions & Role-Permissions (Sprint 1)
-- ------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS permissions (
    id TEXT PRIMARY KEY, -- store UUID as TEXT
    name TEXT UNIQUE NOT NULL,
    description TEXT,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS role_permissions (
    role_id TEXT NOT NULL,
    permission_id TEXT NOT NULL,
    granted_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (role_id, permission_id),
    FOREIGN KEY(role_id) REFERENCES roles(id) ON DELETE CASCADE,
    FOREIGN KEY(permission_id) REFERENCES permissions(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_role_permissions_role_id ON role_permissions(role_id);
CREATE INDEX IF NOT EXISTS idx_role_permissions_permission_id ON role_permissions(permission_id);

-- ------------------------------------------------------------------
-- 2. Password History (Sprint 1)
-- ------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS password_history (
    id TEXT PRIMARY KEY,
    user_id TEXT NOT NULL,
    password_hash TEXT NOT NULL,
    changed_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_password_history_user_id ON password_history(user_id);

-- ------------------------------------------------------------------
-- 3. RBAC Resolution View (Sprint 1)
-- ------------------------------------------------------------------
CREATE VIEW IF NOT EXISTS v_user_permissions AS
SELECT DISTINCT ur.user_id, p.name AS permission_name
FROM user_roles ur
JOIN role_permissions rp ON ur.role_id = rp.role_id
JOIN permissions p ON rp.permission_id = p.id;

-- ------------------------------------------------------------------
-- Note for SQLite:
-- Partitioning and Stored Procedures are not natively supported in SQLite.
-- The application code handles cleanup tasks for SQLite environments.
-- ------------------------------------------------------------------
