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

export type User = {
  id: string;
  name: string;
  email: string;
  email_verified: boolean;
  active: boolean;
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
  status: string;
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

export type WorkspacesData = { workspaces: Workspace[] };
export type ProjectsData = { projects: Project[] };
export type MembersData = { members: Member[] };
export type TokensData = { tokens: CliToken[] };
export type PackagesData = { packages: Package[] };
export type PackageVersionsData = { package_versions: PackageVersion[] };
export type LockfilesData = { lockfiles: Lockfile[] };
export type BuildReportsData = { build_reports: BuildReport[] };
