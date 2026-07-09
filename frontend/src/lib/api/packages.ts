import { api } from './client';
import type { Package, PackagesData } from './types';

export function listPackages(workspaceId: string) {
  return api.post<PackagesData>('/api/packages/list', { workspace_id: workspaceId });
}

export function createPackage(input: {
  workspaceId: string;
  ownerUserId: string;
  name: string;
  description?: string;
  repositoryUrl?: string;
  visibility?: string;
}) {
  return api.post<{ package: Package }>('/api/packages', {
    workspace_id: input.workspaceId,
    owner_user_id: input.ownerUserId,
    name: input.name,
    description: input.description ?? '',
    repository_url: input.repositoryUrl ?? '',
    visibility: input.visibility ?? 'private'
  });
}
