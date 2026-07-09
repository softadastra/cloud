CREATE TABLE IF NOT EXISTS workspace_invites (
  id TEXT PRIMARY KEY,
  workspace_id TEXT NOT NULL,
  invited_email TEXT NOT NULL,
  invited_user_id TEXT,
  role TEXT NOT NULL,
  invited_by_user_id TEXT NOT NULL,
  status TEXT NOT NULL DEFAULT 'pending',
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,
  expires_at INTEGER,
  FOREIGN KEY (workspace_id) REFERENCES workspaces(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_workspace_invites_workspace
  ON workspace_invites(workspace_id);

CREATE INDEX IF NOT EXISTS idx_workspace_invites_email_status
  ON workspace_invites(invited_email, status);

CREATE INDEX IF NOT EXISTS idx_workspace_invites_user_status
  ON workspace_invites(invited_user_id, status);

CREATE UNIQUE INDEX IF NOT EXISTS idx_workspace_invites_pending_unique
  ON workspace_invites(workspace_id, invited_email)
  WHERE status = 'pending';
