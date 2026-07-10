CREATE TABLE IF NOT EXISTS public_activity_events (
  id TEXT PRIMARY KEY,
  user_id TEXT NOT NULL,
  workspace_id TEXT,
  project_id TEXT,
  package_id TEXT,
  type TEXT NOT NULL,
  title TEXT NOT NULL,
  data_json TEXT,
  visibility TEXT NOT NULL DEFAULT 'public' CHECK (visibility IN ('public')),
  created_at INTEGER NOT NULL,
  FOREIGN KEY(user_id) REFERENCES rix_auth_users(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_public_activity_events_user_id
  ON public_activity_events(user_id);

CREATE INDEX IF NOT EXISTS idx_public_activity_events_package_id
  ON public_activity_events(package_id);
