import { api } from './client';
import type { LockfilesData } from './types';

export function listLockfiles(workspaceId: string, projectId: string) {
  return api.post<LockfilesData>('/api/lockfiles/list', {
    workspace_id: workspaceId,
    project_id: projectId
  });
}
