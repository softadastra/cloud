CREATE TABLE IF NOT EXISTS projects (
  id TEXT PRIMARY KEY,
  workspace_id TEXT NOT NULL,
  owner_user_id TEXT NOT NULL,
  name TEXT NOT NULL,
  slug TEXT NOT NULL,
  description TEXT,
  repository_url TEXT,
  default_branch TEXT NOT NULL DEFAULT 'main',
  active INTEGER NOT NULL DEFAULT 1,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,
  FOREIGN KEY (workspace_id) REFERENCES workspaces(id) ON DELETE CASCADE,
  UNIQUE (workspace_id, slug)
);

CREATE INDEX IF NOT EXISTS idx_projects_workspace_id
  ON projects(workspace_id);

CREATE INDEX IF NOT EXISTS idx_projects_owner_user_id
  ON projects(owner_user_id);
