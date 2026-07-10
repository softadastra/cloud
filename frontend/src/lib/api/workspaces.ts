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


export function updateWorkspace(input: { id: string; name: string; slug: string }) {
  return api.post<{ workspace: Workspace }>('/api/workspaces/update', input);
}

export function suspendWorkspace(id: string) {
  return api.post<{ workspace: Workspace }>('/api/workspaces/suspend', { id });
}

export function reactivateWorkspace(id: string) {
  return api.post<{ workspace: Workspace }>('/api/workspaces/reactivate', { id });
}

export function deleteWorkspace(id: string) {
  return api.post<{ workspace: Workspace }>('/api/workspaces/delete', { id });
}

export function uploadWorkspaceAvatar(workspaceId: string, file: File) {
  return api.postRaw<{ avatar_url: string; workspace: Workspace }>(
    `/api/workspaces/avatar/upload?workspace_id=${encodeURIComponent(workspaceId)}`,
    file,
    file.type
  );
}

export function deleteWorkspaceAvatar(id: string) {
  return api.post<{ message: string }>('/api/workspaces/avatar/delete', { id });
}
