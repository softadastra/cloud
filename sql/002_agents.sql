-- 002_agents.sql
--
-- Softadastra Cloud agents schema.
--
-- Agents represent CLI/runtime instances allowed to submit runs, events,
-- and reports for a project.

CREATE TABLE IF NOT EXISTS agents (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  public_id TEXT NOT NULL UNIQUE,
  project_id INTEGER NOT NULL,
  name TEXT NOT NULL,
  api_key_hash TEXT NOT NULL,
  last_seen_at INTEGER,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,

  FOREIGN KEY(project_id) REFERENCES projects(id)
);

CREATE INDEX IF NOT EXISTS idx_agents_public_id
  ON agents(public_id);

CREATE INDEX IF NOT EXISTS idx_agents_project_id
  ON agents(project_id);

CREATE INDEX IF NOT EXISTS idx_agents_api_key_hash
  ON agents(api_key_hash);

CREATE INDEX IF NOT EXISTS idx_agents_last_seen_at
  ON agents(last_seen_at);
