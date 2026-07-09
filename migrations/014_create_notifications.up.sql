CREATE TABLE IF NOT EXISTS notifications (
  id TEXT PRIMARY KEY,
  workspace_id TEXT,
  project_id TEXT,
  recipient_user_id TEXT NOT NULL,
  actor_user_id TEXT,
  type TEXT NOT NULL,
  title TEXT NOT NULL,
  message TEXT NOT NULL,
  data_json TEXT,
  read_at INTEGER,
  created_at INTEGER NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_notifications_recipient_created
  ON notifications(recipient_user_id, created_at);

CREATE INDEX IF NOT EXISTS idx_notifications_recipient_read
  ON notifications(recipient_user_id, read_at);

CREATE INDEX IF NOT EXISTS idx_notifications_workspace
  ON notifications(workspace_id, created_at);

CREATE TABLE IF NOT EXISTS notification_preferences (
  id TEXT PRIMARY KEY,
  user_id TEXT NOT NULL,
  workspace_id TEXT,
  notify_invites INTEGER NOT NULL DEFAULT 1,
  notify_member_changes INTEGER NOT NULL DEFAULT 1,
  notify_tokens INTEGER NOT NULL DEFAULT 1,
  notify_projects INTEGER NOT NULL DEFAULT 1,
  notify_packages INTEGER NOT NULL DEFAULT 1,
  notify_lockfiles INTEGER NOT NULL DEFAULT 1,
  notify_build_reports INTEGER NOT NULL DEFAULT 1,
  notify_security INTEGER NOT NULL DEFAULT 1,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,
  UNIQUE(user_id, workspace_id)
);
