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


export function updatePackage(input: {
  workspaceId: string;
  id: string;
  name: string;
  description?: string;
  repositoryUrl?: string;
  visibility?: string;
}) {
  return api.post<{ package: Package }>('/api/packages/update', {
    workspace_id: input.workspaceId,
    id: input.id,
    name: input.name,
    description: input.description ?? '',
    repository_url: input.repositoryUrl ?? '',
    visibility: input.visibility ?? 'private'
  });
}

export function archivePackage(workspaceId: string, id: string) {
  return api.post<{ package: Package }>('/api/packages/archive', { workspace_id: workspaceId, id });
}

export function reactivatePackage(workspaceId: string, id: string) {
  return api.post<{ package: Package }>('/api/packages/reactivate', { workspace_id: workspaceId, id });
}

export function deletePackage(workspaceId: string, id: string) {
  return api.post<{ package: Package }>('/api/packages/delete', { workspace_id: workspaceId, id });
}

export function changePackageVisibility(workspaceId: string, id: string, visibility: 'private' | 'public') {
  return api.post<{ package: Package }>('/api/packages/change_visibility', { workspace_id: workspaceId, id, visibility });
}
