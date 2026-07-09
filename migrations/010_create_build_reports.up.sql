CREATE TABLE IF NOT EXISTS build_reports (
  id TEXT PRIMARY KEY,
  workspace_id TEXT NOT NULL,
  project_id TEXT NOT NULL,
  submitted_by_user_id TEXT NOT NULL,
  status TEXT NOT NULL CHECK (status IN ('success', 'failed', 'warning', 'cancelled')),
  target TEXT NOT NULL,
  profile TEXT NOT NULL,
  branch TEXT,
  commit_sha TEXT,
  toolchain TEXT,
  summary_json TEXT NOT NULL,
  diagnostics_json TEXT NOT NULL,
  duration_ms INTEGER NOT NULL DEFAULT 0,
  warnings_count INTEGER NOT NULL DEFAULT 0,
  errors_count INTEGER NOT NULL DEFAULT 0,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,
  FOREIGN KEY (workspace_id) REFERENCES workspaces(id) ON DELETE CASCADE,
  FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_build_reports_workspace_id
  ON build_reports(workspace_id);

CREATE INDEX IF NOT EXISTS idx_build_reports_project_id
  ON build_reports(project_id);

CREATE INDEX IF NOT EXISTS idx_build_reports_created_at
  ON build_reports(created_at);
