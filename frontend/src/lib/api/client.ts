import { browser } from '$app/environment';
import { ApiError, type ApiFailure, type ApiResponse } from './types';

export type ApiClientOptions = {
  baseUrl?: string;
  sessionId?: string | null;
  authHeader?: 'authorization' | 'x-session-id' | 'both';
};

const DEFAULT_BASE_URL = import.meta.env.VITE_API_BASE_URL ?? (import.meta.env.DEV && browser ? `${window.location.protocol}//${window.location.hostname}:8080` : '');
const AUTH_STORAGE_KEY = 'softadastra.cloud.auth';

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

  private restoreSessionFromStorage() {
    if (this.sessionId || !browser) {
      return;
    }

    const raw = localStorage.getItem(AUTH_STORAGE_KEY);

    if (!raw) {
      return;
    }

    try {
      const parsed = JSON.parse(raw) as { session?: { id?: string } };
      this.sessionId = parsed.session?.id ?? null;
    } catch {
      this.sessionId = null;
    }
  }

  async get<T>(path: string): Promise<T> {
    return this.request<T>('GET', path);
  }

  async post<T>(path: string, body: Record<string, unknown>): Promise<T> {
    return this.request<T>('POST', path, body);
  }

  private async request<T>(method: string, path: string, body?: Record<string, unknown>): Promise<T> {
    this.restoreSessionFromStorage();

    const headers: Record<string, string> = {
      Accept: 'application/json'
    };

    const init: RequestInit = {
      method,
      headers
    };

    if (body !== undefined) {
      headers['Content-Type'] = 'application/json; charset=utf-8';
      init.body = JSON.stringify(body);
    }

    if (this.sessionId) {
      if (this.authHeader === 'authorization' || this.authHeader === 'both') {
        headers.Authorization = `Bearer ${this.sessionId}`;
      }

      if (this.authHeader === 'x-session-id' || this.authHeader === 'both') {
        headers['X-Session-Id'] = this.sessionId;
      }
    }

    let response: Response;

    try {
      response = await fetch(`${this.baseUrl}${path}`, init);
    } catch {
      throw new ApiError(
        0,
        'network_error',
        'Unable to reach the API. Check that the backend is running and that VITE_API_BASE_URL is correct.'
      );
    }
    const text = await response.text();
    let payload: ApiResponse<T> | null = null;

    try {
      payload = text ? (JSON.parse(text) as ApiResponse<T>) : null;
    } catch {
      throw new ApiError(response.status, 'invalid_json_response', `Invalid JSON response from ${path}.`);
    }

    if (!response.ok || isApiFailure(payload)) {
      const failure = isApiFailure(payload)
        ? payload
        : { ok: false as const, error: 'http_error', message: `HTTP ${response.status}` };
      throw new ApiError(
        response.status,
        failure.error,
        response.status === 401 ? 'Session expired. Please log in again.' : failure.message
      );
    }

    if (!payload || payload.ok !== true || !('data' in payload)) {
      throw new ApiError(response.status, 'invalid_response', 'Invalid API response.');
    }

    return payload.data;
  }
}

export const api = new ApiClient({ authHeader: 'both' });
