import { api } from './client';
import type { Workspace, WorkspacesData } from './types';

export function listWorkspaces(ownerUserId: string) {
  return api.post<WorkspacesData>('/api/workspaces/list', { owner_user_id: ownerUserId });
}

export function createWorkspace(input: { name: string; slug: string; ownerUserId: string }) {
  return api.post<{ workspace: Workspace }>('/api/workspaces', {
    name: input.name,
    slug: input.slug,
    owner_user_id: input.ownerUserId
  });
}
