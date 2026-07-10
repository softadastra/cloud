import { api } from './client';
import type { LoginData, PlatformAdminInfo, RegisterData, UpdateProfilePayload, User } from './types';

export function login(email: string, password: string) {
  return api.post<LoginData>('/api/auth/login', { email, password });
}

export function register(name: string, email: string, password: string) {
  return api.post<RegisterData>('/api/auth/register', { name, email, password });
}


export function me(sessionId: string) {
  return api.post<{ user: User; platform_admin?: PlatformAdminInfo; session: LoginData['session'] }>('/api/auth/me', { session_id: sessionId });
}

export function updateProfile(sessionId: string, payload: UpdateProfilePayload) {
  return api.post<{ user: User }>('/api/auth/update_profile', {
    session_id: sessionId,
    display_name: payload.display_name,
    username: payload.username,
    bio: payload.bio,
    website_url: payload.website_url,
    github_url: payload.github_url,
    public_profile_enabled: payload.public_profile_enabled
  });
}

export function uploadAvatar(file: File) {
  return api.postRaw<{ avatar_url: string; user: User }>(
    '/api/auth/avatar/upload',
    file,
    file.type
  );
}

export function deleteAvatar() {
  return api.post<{ message: string }>('/api/auth/avatar/delete', {});
}

export function changePassword(input: { sessionId: string; currentPassword: string; newPassword: string; confirmNewPassword: string }) {
  return api.post<{ message: string }>('/api/auth/change_password', {
    session_id: input.sessionId,
    current_password: input.currentPassword,
    new_password: input.newPassword,
    confirm_new_password: input.confirmNewPassword
  });
}
