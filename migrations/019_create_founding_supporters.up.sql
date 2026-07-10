CREATE TABLE IF NOT EXISTS founding_supporters (
  id TEXT PRIMARY KEY,
  user_id TEXT NULL,
  tier TEXT NOT NULL CHECK (tier IN ('founding_supporter', 'founding_builder')),
  status TEXT NOT NULL DEFAULT 'active' CHECK (status IN ('active', 'inactive', 'cancelled')),
  display_name TEXT NOT NULL,
  username TEXT NULL,
  project_name TEXT NULL,
  website_url TEXT NULL,
  github_url TEXT NULL,
  public_visible INTEGER NOT NULL DEFAULT 1,
  stronger_visibility INTEGER NOT NULL DEFAULT 0,
  notes TEXT NULL,
  started_at INTEGER NOT NULL,
  expires_at INTEGER NULL,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_founding_supporters_user_id ON founding_supporters(user_id);
CREATE INDEX IF NOT EXISTS idx_founding_supporters_public ON founding_supporters(status, public_visible, tier);
