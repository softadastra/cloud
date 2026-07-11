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
    this.name = "ApiError";
    this.status = status;
    this.code = code;
  }
}


export type PlatformAdminInfo = {
  role: 'owner' | 'admin' | 'viewer' | string;
} | null;

export type AdminModule = {
  key: string;
  name: string;
  description: string;
  actions: string[];
};

export type AdminOverview = {
  platform_admin: PlatformAdminInfo;
  stats: Record<string, number>;
  recent_activity: AdminAuditLog[];
  recent_feedback: AdminFeedbackSummary[];
  recent_supporters: AdminSupporterSummary[];
};

export type AdminUserSummary = {
  id: string;
  email: string;
  active: boolean;
  created_at: number;
  display_name: string;
  username: string;
  public_profile_enabled: boolean;
  supporter_tier: string;
  supporter_status: string;
};

export type AdminPackageSummary = {
  id: string;
  workspace_id: string;
  owner_user_id: string;
  name: string;
  description: string;
  repository_url: string;
  visibility: string;
  status?: string;
  active: boolean;
  archived_at?: number;
  deleted_at?: number;
  created_at: number;
  updated_at: number;
  workspace_name: string;
  owner_email: string;
};

export type AdminFeedbackSummary = {
  id: string;
  user_id: string;
  user_email?: string;
  workspace_id: string;
  category: string;
  title: string;
  message?: string;
  status: string;
  created_at: number;
  updated_at?: number;
};

export type AdminSupporterSummary = {
  id: string;
  user_id: string;
  tier: SupportPlanTier | string;
  status: string;
  display_name: string;
  username: string;
  project_name: string;
  website_url: string;
  github_url: string;
  public_visible: boolean;
  stronger_visibility: boolean;
  started_at: number;
  expires_at: number;
  created_at: number;
  updated_at: number;
  deleted_at?: number;
};

export type AdminSupporterInput = Partial<AdminSupporterSummary> & {
  display_name: string;
  tier: string;
};

export type AdminAuditLog = {
  id: string;
  admin_user_id: string;
  action: string;
  target_type: string;
  target_id: string;
  metadata_json: string;
  ip_address: string;
  user_agent: string;
  created_at: number;
};

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
  platform_admin?: PlatformAdminInfo;
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
  namespace?: string;
  name: string;
  full_name?: string;
  description: string;
  repository_url: string;
  visibility: "public" | string;
  created_at: number;
  updated_at: number;
  latest_version?: string;
  versions_count?: number;
  package_type?: string;
};

export type PublicPackageOwner = {
  display_name: string;
  username: string;
  avatar_url: string;
  avatar_updated_at: number;
  supporter?: {
    tier: string;
  } | null;
};

export type PublicPackageStats = {
  versions_count: number;
  active_versions_count: number;
  latest_version: string;
};

export type PublicPackageVersionSummary = {
  id: string;
  version: string;
  status: "active" | "deprecated" | "yanked" | string;
  checksum: string;
  archive_size: number;
  created_at: number;
  deprecated_at: number | null;
  deprecation_message: string | null;
  yanked_at: number | null;
};

export type PublicPackageActivityEvent = PublicActivityEvent;

export type PublicPackageDetail = {
  id: string;
  namespace: string;
  name: string;
  description: string;
  visibility: "public" | string;
  status: "active" | "archived" | string;
  repository_url: string;
  created_at: number;
  updated_at: number;
  owner: PublicPackageOwner;
  stats: PublicPackageStats;
  versions: PublicPackageVersionSummary[];
  recent_activity: PublicPackageActivityEvent[];
};

export type PublicPackageShowResponse = {
  package: PublicPackageDetail;
};

export type PublicActivityEvent = {
  id: string;
  package_id: string;
  type: string;
  title: string;
  data_json: string;
  created_at: number;
};

export type ContributionGridDay = {
  date: string;
  count: number;
  level: number;
};

export type PublicProfileStats = {
  public_packages_count: number;
  public_contributions_count: number;
  pinned_packages_count: number;
};

export type SupportPlanTier = 'founding_supporter' | 'founding_builder';

export type PublicSupporter = {
  tier: SupportPlanTier | string;
  display_name: string;
  username: string;
  project_name: string;
  website_url: string;
  github_url: string;
  stronger_visibility: boolean;
  started_at: number;
};

export type SupportConfig = {
  supporter_payment_url: string;
  builder_payment_url: string;
  contact_email: string;
  contact_url: string;
};

export type MySupporterStatus = {
  supporter: (PublicSupporter & {
    status: string;
    public_visible: boolean;
  }) | null;
};

export type PublicSupportersData = {
  supporters: PublicSupporter[];
};

export type PublicProfileResponse = {
  profile: PublicProfile;
  supporter?: PublicSupporter | null;
  pinned_packages: PublicPackageSummary[];
  public_packages: PublicPackageSummary[];
  contribution_grid: ContributionGridDay[];
  contribution_years: number[];
  selected_contribution_year: number;
  recent_activity: PublicActivityEvent[];
  public_activity?: PublicActivityEvent[];
  stats: PublicProfileStats;
};

export type ProfilePinsData = {
  package_ids: string[];
  pinned_packages: PublicPackageSummary[];
};

export type Session = {
  id: string;
  expires_at: number;
};

export type LoginData = {
  user: User;
  platform_admin?: PlatformAdminInfo;
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
  current_user_role?: "owner" | "admin" | "member" | "viewer" | string;
  current_user_status?: "active" | "suspended" | "removed" | string;
  access_scope?: "entire_workspace" | "selected_projects" | string;
  project_ids_json?: string;
  current_user_is_owner?: boolean;
  status?: string;
  avatar_url?: string;
  avatar_storage_path?: string;
  avatar_updated_at?: number;
  suspended_at?: number;
  deleted_at?: number;
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
  status?: string;
  active: boolean;
  archived_at?: number;
  deleted_at?: number;
  created_at: number;
  updated_at: number;
};

export type Member = {
  id: string;
  workspace_id: string;
  user_id: string;
  email: string;
  role: "owner" | "admin" | "member" | "viewer" | string;
  status: "active" | "suspended" | "removed" | string;
  access_scope?: "entire_workspace" | "selected_projects" | string;
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
  status?: string;
  active: boolean;
  archived_at?: number;
  deleted_at?: number;
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
  yanked_at?: number;
  deprecated_at?: number;
  deprecation_message?: string;
  deleted_at?: number;
  access_scope?: "entire_workspace" | "selected_projects" | string;
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
  status?: string;
  deleted_at?: number;
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
  record_status?: string;
  deleted_at?: number;
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
  access_scope?: "entire_workspace" | "selected_projects" | string;
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
  deleted_at?: number;
};
