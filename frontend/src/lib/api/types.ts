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

export type WorkspacesData = {
  workspaces: Workspace[];
};
