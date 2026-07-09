import { api } from './client';
import type { LoginData, RegisterData } from './types';

export function login(email: string, password: string) {
  return api.post<LoginData>('/api/auth/login', { email, password });
}

export function register(name: string, email: string, password: string) {
  return api.post<RegisterData>('/api/auth/register', { name, email, password });
}
