import { api } from './client';
import type { LoginData, RegisterData, User } from './types';

export function login(email: string, password: string) {
  return api.post<LoginData>('/api/auth/login', { email, password });
}

export function register(name: string, email: string, password: string) {
  return api.post<RegisterData>('/api/auth/register', { name, email, password });
}


export function me(sessionId: string) {
  return api.post<{ user: User; session: LoginData['session'] }>('/api/auth/me', { session_id: sessionId });
}

export function updateProfile(sessionId: string, displayName: string) {
  return api.post<{ user: User }>('/api/auth/update_profile', { session_id: sessionId, display_name: displayName });
}

export function changePassword(input: { sessionId: string; currentPassword: string; newPassword: string; confirmNewPassword: string }) {
  return api.post<{ message: string }>('/api/auth/change_password', {
    session_id: input.sessionId,
    current_password: input.currentPassword,
    new_password: input.newPassword,
    confirm_new_password: input.confirmNewPassword
  });
}
