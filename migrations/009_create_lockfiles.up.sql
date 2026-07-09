CREATE TABLE IF NOT EXISTS lockfiles (
  id TEXT PRIMARY KEY,
  workspace_id TEXT NOT NULL,
  project_id TEXT NOT NULL,
  uploaded_by_user_id TEXT NOT NULL,
  checksum_sha256 TEXT NOT NULL,
  lockfile_json TEXT NOT NULL,
  source TEXT NOT NULL,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,
  FOREIGN KEY (workspace_id) REFERENCES workspaces(id) ON DELETE CASCADE,
  FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_lockfiles_workspace_id
  ON lockfiles(workspace_id);

CREATE INDEX IF NOT EXISTS idx_lockfiles_project_id
  ON lockfiles(project_id);

CREATE INDEX IF NOT EXISTS idx_lockfiles_created_at
  ON lockfiles(created_at);
