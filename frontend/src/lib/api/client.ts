import { ApiError, type ApiFailure, type ApiResponse } from './types';

export type ApiClientOptions = {
  baseUrl?: string;
  sessionId?: string | null;
  authHeader?: 'authorization' | 'x-session-id' | 'both';
};

const DEFAULT_BASE_URL = '';

function isApiFailure(value: unknown): value is ApiFailure {
  return Boolean(
    value &&
      typeof value === 'object' &&
      (value as { ok?: unknown }).ok === false &&
      typeof (value as { error?: unknown }).error === 'string' &&
      typeof (value as { message?: unknown }).message === 'string'
  );
}

export class ApiClient {
  private readonly baseUrl: string;
  private sessionId: string | null;
  private readonly authHeader: 'authorization' | 'x-session-id' | 'both';

  constructor(options: ApiClientOptions = {}) {
    this.baseUrl = options.baseUrl ?? DEFAULT_BASE_URL;
    this.sessionId = options.sessionId ?? null;
    this.authHeader = options.authHeader ?? 'authorization';
  }

  setSession(sessionId: string | null) {
    this.sessionId = sessionId;
  }

  async get<T>(path: string): Promise<T> {
    return this.request<T>('GET', path);
  }

  async post<T>(path: string, body: unknown): Promise<T> {
    return this.request<T>('POST', path, body);
  }

  private async request<T>(method: string, path: string, body?: unknown): Promise<T> {
    const headers: Record<string, string> = {
      Accept: 'application/json'
    };

    if (body !== undefined) {
      headers['Content-Type'] = 'application/json';
    }

    if (this.sessionId) {
      if (this.authHeader === 'authorization' || this.authHeader === 'both') {
        headers.Authorization = `Bearer ${this.sessionId}`;
      }

      if (this.authHeader === 'x-session-id' || this.authHeader === 'both') {
        headers['X-Session-Id'] = this.sessionId;
      }
    }

    const response = await fetch(`${this.baseUrl}${path}`, {
      method,
      headers,
      body: body === undefined ? undefined : JSON.stringify(body)
    });

    const payload = (await response.json()) as ApiResponse<T>;

    if (!response.ok || isApiFailure(payload)) {
      const failure = isApiFailure(payload)
        ? payload
        : { ok: false as const, error: 'http_error', message: `HTTP ${response.status}` };
      throw new ApiError(response.status, failure.error, failure.message);
    }

    if (payload.ok !== true || !('data' in payload)) {
      throw new ApiError(response.status, 'invalid_response', 'Invalid API response.');
    }

    return payload.data;
  }
}

export const api = new ApiClient({ authHeader: 'both' });
