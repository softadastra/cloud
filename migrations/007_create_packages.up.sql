CREATE TABLE IF NOT EXISTS packages (
  id TEXT PRIMARY KEY,
  workspace_id TEXT NOT NULL,
  owner_user_id TEXT NOT NULL,
  name TEXT NOT NULL,
  description TEXT,
  repository_url TEXT,
  visibility TEXT NOT NULL DEFAULT 'private' CHECK (visibility IN ('private', 'public')),
  active INTEGER NOT NULL DEFAULT 1,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,
  FOREIGN KEY (workspace_id) REFERENCES workspaces(id) ON DELETE CASCADE,
  UNIQUE (workspace_id, name)
);

CREATE INDEX IF NOT EXISTS idx_packages_workspace_id
  ON packages(workspace_id);

CREATE INDEX IF NOT EXISTS idx_packages_owner_user_id
  ON packages(owner_user_id);
