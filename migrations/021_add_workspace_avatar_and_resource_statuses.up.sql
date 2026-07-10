ALTER TABLE workspaces ADD COLUMN avatar_url TEXT;
ALTER TABLE workspaces ADD COLUMN avatar_storage_path TEXT;
ALTER TABLE workspaces ADD COLUMN avatar_updated_at INTEGER;
ALTER TABLE workspaces ADD COLUMN status TEXT NOT NULL DEFAULT 'active';
ALTER TABLE workspaces ADD COLUMN suspended_at INTEGER;
ALTER TABLE workspaces ADD COLUMN deleted_at INTEGER;

ALTER TABLE projects ADD COLUMN status TEXT NOT NULL DEFAULT 'active';
ALTER TABLE projects ADD COLUMN archived_at INTEGER;
ALTER TABLE projects ADD COLUMN deleted_at INTEGER;

ALTER TABLE packages ADD COLUMN status TEXT NOT NULL DEFAULT 'active';
ALTER TABLE packages ADD COLUMN archived_at INTEGER;
ALTER TABLE packages ADD COLUMN deleted_at INTEGER;

ALTER TABLE package_versions ADD COLUMN deprecated_at INTEGER;
ALTER TABLE package_versions ADD COLUMN deprecation_message TEXT;
ALTER TABLE package_versions ADD COLUMN deleted_at INTEGER;

ALTER TABLE lockfiles ADD COLUMN status TEXT NOT NULL DEFAULT 'active';
ALTER TABLE lockfiles ADD COLUMN deleted_at INTEGER;

ALTER TABLE build_reports ADD COLUMN record_status TEXT NOT NULL DEFAULT 'active';
ALTER TABLE build_reports ADD COLUMN deleted_at INTEGER;

ALTER TABLE feedback_items ADD COLUMN deleted_at INTEGER;
