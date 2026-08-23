-- migrations/002_pg_schema_completeness.sql
-- Adds missing tables (permissions, role_permissions, password_history)
-- Implements RBAC Views, Audit Log Partitioning, and Stored Procedures

-- ------------------------------------------------------------------
-- 1. Missing Tables: Permissions & Role-Permissions (Sprint 1)
-- ------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS permissions (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    name VARCHAR(100) UNIQUE NOT NULL,
    description TEXT,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS role_permissions (
    role_id UUID NOT NULL REFERENCES roles(id) ON DELETE CASCADE,
    permission_id UUID NOT NULL REFERENCES permissions(id) ON DELETE CASCADE,
    granted_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (role_id, permission_id)
);

CREATE INDEX IF NOT EXISTS idx_role_permissions_role_id ON role_permissions(role_id);
CREATE INDEX IF NOT EXISTS idx_role_permissions_permission_id ON role_permissions(permission_id);

-- ------------------------------------------------------------------
-- 2. Password History (Sprint 1)
-- ------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS password_history (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    password_hash VARCHAR(255) NOT NULL,
    changed_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_password_history_user_id ON password_history(user_id);

-- ------------------------------------------------------------------
-- 3. RBAC Resolution View (Sprint 1)
-- ------------------------------------------------------------------
CREATE OR REPLACE VIEW v_user_permissions AS
SELECT DISTINCT ur.user_id, p.name AS permission_name
FROM user_roles ur
JOIN role_permissions rp ON ur.role_id = rp.role_id
JOIN permissions p ON rp.permission_id = p.id;

-- ------------------------------------------------------------------
-- 4. Audit Log Partitioning (Sprint 1)
-- ------------------------------------------------------------------
-- Rename the old non-partitioned table
ALTER TABLE audit_logs RENAME TO audit_logs_old;

-- Create the new partitioned table (PK must include partition key)
CREATE TABLE audit_logs (
    id UUID DEFAULT gen_random_uuid(),
    user_id UUID REFERENCES users(id) ON DELETE SET NULL,
    action VARCHAR(100) NOT NULL,
    ip_address VARCHAR(45),
    details JSONB DEFAULT '{}'::jsonb,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id, created_at)
) PARTITION BY RANGE (created_at);

CREATE INDEX IF NOT EXISTS idx_audit_logs_user_id_part ON audit_logs(user_id);
CREATE INDEX IF NOT EXISTS idx_audit_logs_created_at_part ON audit_logs(created_at);

-- Create initial partitions (e.g. for the rest of 2026)
CREATE TABLE audit_logs_y2026m08 PARTITION OF audit_logs FOR VALUES FROM ('2026-08-01') TO ('2026-09-01');
CREATE TABLE audit_logs_y2026m09 PARTITION OF audit_logs FOR VALUES FROM ('2026-09-01') TO ('2026-10-01');
CREATE TABLE audit_logs_y2026m10 PARTITION OF audit_logs FOR VALUES FROM ('2026-10-01') TO ('2026-11-01');
CREATE TABLE audit_logs_y2026m11 PARTITION OF audit_logs FOR VALUES FROM ('2026-11-01') TO ('2026-12-01');
CREATE TABLE audit_logs_y2026m12 PARTITION OF audit_logs FOR VALUES FROM ('2026-12-01') TO ('2027-01-01');

-- Migrate data from old table to new partitioned table
INSERT INTO audit_logs (id, user_id, action, ip_address, details, created_at)
SELECT id, user_id, action, ip_address, details, created_at FROM audit_logs_old;

-- Drop old table
DROP TABLE audit_logs_old;

-- ------------------------------------------------------------------
-- 5. Stored Procedures (Sprint 1)
-- ------------------------------------------------------------------
-- Cleanup expired sessions
CREATE OR REPLACE PROCEDURE sp_cleanup_expired_sessions()
LANGUAGE plpgsql
AS $$
BEGIN
    DELETE FROM sessions WHERE expires_at < CURRENT_TIMESTAMP;
END;
$$;

-- Force password reset for a user
CREATE OR REPLACE PROCEDURE sp_force_password_reset(p_user_id UUID)
LANGUAGE plpgsql
AS $$
BEGIN
    UPDATE users SET must_pwd_change = TRUE WHERE id = p_user_id;
    -- Optionally invalidate their sessions
    DELETE FROM sessions WHERE user_id = p_user_id;
END;
$$;
