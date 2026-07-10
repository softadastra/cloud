CREATE TABLE IF NOT EXISTS public_profile_pins (
  id TEXT PRIMARY KEY,
  user_id TEXT NOT NULL,
  package_id TEXT NOT NULL,
  sort_order INTEGER NOT NULL DEFAULT 0,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,
  FOREIGN KEY(user_id) REFERENCES rix_auth_users(id) ON DELETE CASCADE,
  FOREIGN KEY(package_id) REFERENCES packages(id) ON DELETE CASCADE,
  UNIQUE(user_id, package_id)
);

CREATE INDEX IF NOT EXISTS idx_public_profile_pins_user_id
  ON public_profile_pins(user_id);

CREATE INDEX IF NOT EXISTS idx_public_profile_pins_package_id
  ON public_profile_pins(package_id);
