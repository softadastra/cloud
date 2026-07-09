ALTER TABLE workspace_members ADD COLUMN access_scope TEXT NOT NULL DEFAULT 'entire_workspace';
ALTER TABLE workspace_members ADD COLUMN project_ids_json TEXT;
ALTER TABLE workspace_invites ADD COLUMN access_scope TEXT NOT NULL DEFAULT 'entire_workspace';
ALTER TABLE workspace_invites ADD COLUMN project_ids_json TEXT;
