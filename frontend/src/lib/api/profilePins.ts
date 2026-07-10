import { api } from './client';
import type { ProfilePinsData } from './types';

export function listProfilePins() {
  return api.post<ProfilePinsData>('/api/profile/pins/list', {});
}

export function updateProfilePins(packageIds: string[]) {
  return api.post<ProfilePinsData>('/api/profile/pins/update', { package_ids: packageIds });
}
