-- 005_reports.sql
--
-- Softadastra Cloud reports and reliability scores schema.
--
-- Reports store final agent output.
-- Reliability scores store computed risk summaries for each run.

CREATE TABLE IF NOT EXISTS reports (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  public_id TEXT NOT NULL UNIQUE,
  run_id INTEGER NOT NULL UNIQUE,
  summary TEXT NOT NULL,
  raw_json TEXT NOT NULL,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,

  FOREIGN KEY(run_id) REFERENCES runs(id)
);

CREATE INDEX IF NOT EXISTS idx_reports_public_id
  ON reports(public_id);

CREATE INDEX IF NOT EXISTS idx_reports_run_id
  ON reports(run_id);

CREATE TABLE IF NOT EXISTS reliability_scores (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  run_id INTEGER NOT NULL UNIQUE,
  score INTEGER NOT NULL,
  grade TEXT NOT NULL,
  data_loss_risk INTEGER NOT NULL DEFAULT 0,
  duplicate_risk INTEGER NOT NULL DEFAULT 0,
  timeout_risk INTEGER NOT NULL DEFAULT 0,
  recovery_risk INTEGER NOT NULL DEFAULT 0,
  created_at INTEGER NOT NULL,

  FOREIGN KEY(run_id) REFERENCES runs(id)
);

CREATE INDEX IF NOT EXISTS idx_reliability_scores_run_id
  ON reliability_scores(run_id);

CREATE INDEX IF NOT EXISTS idx_reliability_scores_score
  ON reliability_scores(score);

CREATE INDEX IF NOT EXISTS idx_reliability_scores_grade
  ON reliability_scores(grade);
