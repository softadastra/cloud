export type ApiSuccess<T> = {
  ok: true;
  data: T;
};

export type ApiFailure = {
  ok: false;
  error: string;
  message: string;
};

export type ApiResponse<T> = ApiSuccess<T> | ApiFailure;

export class ApiError extends Error {
  readonly status: number;
  readonly code: string;

  constructor(status: number, code: string, message: string) {
    super(message);
    this.name = 'ApiError';
    this.status = status;
    this.code = code;
  }
}

export type UserProfile = {
  display_name?: string;
  username?: string;
  bio?: string;
  avatar_url?: string;
  website_url?: string;
  github_url?: string;
  public_profile_enabled?: boolean;
};

export type User = UserProfile & {
  id: string;
  name: string;
  email: string;
  email_verified: boolean;
  active: boolean;
  created_at?: number;
};

export type UpdateProfilePayload = {
  display_name: string;
  username: string;
  bio: string;
  website_url: string;
  github_url: string;
  public_profile_enabled: boolean;
};

export type PublicProfile = {
  display_name: string;
  username: string;
  bio: string;
  avatar_url: string;
  website_url: string;
  github_url: string;
  public_profile_enabled: boolean;
};

export type PublicPackageSummary = {
  id: string;
  name: string;
  description: string;
  repository_url: string;
  visibility: 'public' | string;
  created_at: number;
  updated_at: number;
};

export type PublicActivityEvent = {
  id: string;
  package_id: string;
  type: string;
  title: string;
  data_json: string;
  created_at: number;
};

export type PublicProfileResponse = {
  profile: PublicProfile;
  public_packages: PublicPackageSummary[];
  public_activity: PublicActivityEvent[];
  stats: {
    public_packages_count: number;
    public_contributions_count: number;
  };
};

export type Session = {
  id: string;
  expires_at: number;
};

export type LoginData = {
  user: User;
  session: Session;
  token?: {
    value: string;
    issuer: string;
    expires_at: number;
  };
};

export type RegisterData = {
  user: User;
};

export type Workspace = {
  id: string;
  name: string;
  slug: string;
  owner_user_id: string;
  current_user_role?: 'owner' | 'admin' | 'member' | 'viewer' | string;
  current_user_status?: 'active' | 'suspended' | 'removed' | string;
  access_scope?: 'entire_workspace' | 'selected_projects' | string;
  project_ids_json?: string;
  current_user_is_owner?: boolean;
  active: boolean;
  created_at: number;
  updated_at: number;
};

export type Project = {
  id: string;
  workspace_id: string;
  owner_user_id: string;
  name: string;
  slug: string;
  description: string;
  repository_url: string;
  default_branch: string;
  active: boolean;
  created_at: number;
  updated_at: number;
};

export type Member = {
  id: string;
  workspace_id: string;
  user_id: string;
  email: string;
  role: 'owner' | 'admin' | 'member' | 'viewer' | string;
  status: 'active' | 'suspended' | 'removed' | string;
  access_scope?: 'entire_workspace' | 'selected_projects' | string;
  project_ids_json?: string;
  invited_by_user_id: string;
  created_at: number;
  updated_at: number;
};

export type CliToken = {
  id: string;
  workspace_id: string;
  user_id: string;
  name: string;
  value: string;
  scopes: string[];
  revoked: boolean;
  created_at: number;
  updated_at: number;
  expires_at: number;
};

export type Package = {
  id: string;
  workspace_id: string;
  owner_user_id: string;
  name: string;
  description: string;
  repository_url: string;
  visibility: string;
  active: boolean;
  created_at: number;
  updated_at: number;
};

export type PackageVersion = {
  id: string;
  workspace_id: string;
  package_id: string;
  published_by_user_id: string;
  version: string;
  archive_url: string;
  checksum_sha256: string;
  manifest_json: string;
  size_bytes: number;
  status: string;
  access_scope?: 'entire_workspace' | 'selected_projects' | string;
  project_ids_json?: string;
  created_at: number;
  updated_at: number;
};

export type Lockfile = {
  id: string;
  workspace_id: string;
  project_id: string;
  uploaded_by_user_id: string;
  checksum_sha256: string;
  lockfile_json: string;
  source: string;
  created_at: number;
};

export type BuildReport = {
  id: string;
  workspace_id: string;
  project_id: string;
  submitted_by_user_id: string;
  status: string;
  target: string;
  profile: string;
  branch: string;
  commit_sha: string;
  toolchain: string;
  summary_json: string;
  diagnostics_json: string;
  duration_ms: number;
  warnings_count: number;
  errors_count: number;
  created_at: number;
};


export type WorkspaceInvite = {
  id: string;
  workspace_id: string;
  workspace_name: string;
  invited_email: string;
  invited_user_id: string;
  role: string;
  invited_by_user_id: string;
  status: string;
  access_scope?: 'entire_workspace' | 'selected_projects' | string;
  project_ids_json?: string;
  created_at: number;
  updated_at: number;
  expires_at: number;
};

export type WorkspacesData = { workspaces: Workspace[] };
export type ProjectsData = { projects: Project[] };
export type MembersData = { members: Member[] };
export type TokensData = { tokens: CliToken[] };
export type PackagesData = { packages: Package[] };
export type PackageVersionsData = { package_versions: PackageVersion[] };
export type LockfilesData = { lockfiles: Lockfile[] };
export type BuildReportsData = { build_reports: BuildReport[] };
export type WorkspaceInvitesData = { invites: WorkspaceInvite[] };


export type NotificationItem = {
  id: string;
  workspace_id: string;
  project_id: string;
  recipient_user_id: string;
  actor_user_id: string;
  type: string;
  title: string;
  message: string;
  data_json: string;
  read_at: number;
  created_at: number;
};

export type NotificationPreferences = {
  id: string;
  user_id: string;
  workspace_id: string;
  notify_invites: boolean;
  notify_member_changes: boolean;
  notify_tokens: boolean;
  notify_projects: boolean;
  notify_packages: boolean;
  notify_lockfiles: boolean;
  notify_build_reports: boolean;
  notify_security: boolean;
  created_at: number;
  updated_at: number;
};

export type FeedbackItem = {
  id: string;
  user_id: string;
  workspace_id: string;
  category: string;
  title: string;
  message: string;
  status: string;
  created_at: number;
  updated_at: number;
};
