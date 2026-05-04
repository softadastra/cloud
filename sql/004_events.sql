-- 004_events.sql
--
-- Softadastra Cloud run events schema.
--
-- Run events store the observable steps produced by agents during
-- reliability tests.

CREATE TABLE IF NOT EXISTS run_events (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  public_id TEXT NOT NULL UNIQUE,
  run_id INTEGER NOT NULL,
  event_type TEXT NOT NULL,
  severity TEXT NOT NULL DEFAULT 'info',
  message TEXT,
  payload TEXT,
  created_at INTEGER NOT NULL,

  FOREIGN KEY(run_id) REFERENCES runs(id)
);

CREATE INDEX IF NOT EXISTS idx_run_events_public_id
  ON run_events(public_id);

CREATE INDEX IF NOT EXISTS idx_run_events_run_id
  ON run_events(run_id);

CREATE INDEX IF NOT EXISTS idx_run_events_event_type
  ON run_events(event_type);

CREATE INDEX IF NOT EXISTS idx_run_events_severity
  ON run_events(severity);

CREATE INDEX IF NOT EXISTS idx_run_events_created_at
  ON run_events(created_at);
