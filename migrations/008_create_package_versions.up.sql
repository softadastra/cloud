CREATE TABLE IF NOT EXISTS package_versions (
  id TEXT PRIMARY KEY,
  workspace_id TEXT NOT NULL,
  package_id TEXT NOT NULL,
  published_by_user_id TEXT NOT NULL,
  version TEXT NOT NULL,
  archive_url TEXT NOT NULL,
  checksum_sha256 TEXT NOT NULL,
  manifest_json TEXT NOT NULL,
  status TEXT NOT NULL DEFAULT 'published',
  size_bytes INTEGER NOT NULL,
  yanked_at INTEGER,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,
  FOREIGN KEY (workspace_id) REFERENCES workspaces(id) ON DELETE CASCADE,
  FOREIGN KEY (package_id) REFERENCES packages(id) ON DELETE CASCADE,
  UNIQUE (package_id, version)
);

CREATE INDEX IF NOT EXISTS idx_package_versions_workspace_id
  ON package_versions(workspace_id);

CREATE INDEX IF NOT EXISTS idx_package_versions_package_id
  ON package_versions(package_id);
