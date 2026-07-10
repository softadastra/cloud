import { api } from './client';
import type { PublicProfileResponse } from './types';

export function getPublicProfile(username: string) {
  return api.post<PublicProfileResponse>('/api/public/users/show', { username });
}
