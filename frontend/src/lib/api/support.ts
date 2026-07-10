import { api } from './client';
import type { MySupporterStatus, PublicSupportersData, SupportConfig } from './types';

export function getSupportConfig() {
  return api.post<SupportConfig>('/api/public/support/config', {});
}

export function listPublicSupporters() {
  return api.post<PublicSupportersData>('/api/public/supporters/list', {});
}

export function getMySupporterStatus() {
  return api.post<MySupporterStatus>('/api/supporters/me', {});
}
