CREATE TABLE IF NOT EXISTS sessions (
  id TEXT PRIMARY KEY,
  user_id TEXT NOT NULL,
  session_hash TEXT NOT NULL UNIQUE,
  expires_at INTEGER NOT NULL,
  revoked_at INTEGER,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,
  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_sessions_user_id
  ON sessions(user_id);

CREATE INDEX IF NOT EXISTS idx_sessions_expires_at
  ON sessions(expires_at);


CREATE TABLE IF NOT EXISTS rix_auth_sessions (
  id VARCHAR(128) NOT NULL PRIMARY KEY,
  user_id VARCHAR(128) NOT NULL,
  created_at BIGINT NOT NULL,
  expires_at BIGINT NOT NULL,
  last_seen_at BIGINT NOT NULL,
  revoked INTEGER NOT NULL DEFAULT 0
);

CREATE INDEX IF NOT EXISTS idx_rix_auth_sessions_user_id
  ON rix_auth_sessions(user_id);

CREATE INDEX IF NOT EXISTS idx_rix_auth_sessions_expires_at
  ON rix_auth_sessions(expires_at);
