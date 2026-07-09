CREATE TABLE IF NOT EXISTS access_tokens (
  id TEXT PRIMARY KEY,
  workspace_id TEXT NOT NULL,
  user_id TEXT NOT NULL,
  name TEXT NOT NULL,
  token_hash TEXT NOT NULL UNIQUE,
  scopes_json TEXT NOT NULL,
  last_used_at INTEGER,
  expires_at INTEGER NOT NULL,
  revoked_at INTEGER,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,
  FOREIGN KEY (workspace_id) REFERENCES workspaces(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_access_tokens_workspace_id
  ON access_tokens(workspace_id);

CREATE INDEX IF NOT EXISTS idx_access_tokens_user_id
  ON access_tokens(user_id);

CREATE INDEX IF NOT EXISTS idx_access_tokens_expires_at
  ON access_tokens(expires_at);
