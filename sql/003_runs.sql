-- 003_runs.sql
--
-- Softadastra Cloud runs schema.
--
-- Runs represent reliability test sessions executed by agents against
-- target API endpoints.

CREATE TABLE IF NOT EXISTS runs (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  public_id TEXT NOT NULL UNIQUE,
  project_id INTEGER NOT NULL,
  agent_id INTEGER,
  status TEXT NOT NULL DEFAULT 'created',
  target_url TEXT NOT NULL,
  method TEXT NOT NULL DEFAULT 'GET',
  scenario TEXT NOT NULL,
  started_at INTEGER NOT NULL,
  finished_at INTEGER,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,

  FOREIGN KEY(project_id) REFERENCES projects(id),
  FOREIGN KEY(agent_id) REFERENCES agents(id)
);

CREATE INDEX IF NOT EXISTS idx_runs_public_id
  ON runs(public_id);

CREATE INDEX IF NOT EXISTS idx_runs_project_id
  ON runs(project_id);

CREATE INDEX IF NOT EXISTS idx_runs_agent_id
  ON runs(agent_id);

CREATE INDEX IF NOT EXISTS idx_runs_status
  ON runs(status);

CREATE INDEX IF NOT EXISTS idx_runs_started_at
  ON runs(started_at);
