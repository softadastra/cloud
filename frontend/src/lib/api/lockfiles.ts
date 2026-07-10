import { api } from './client';
import type { Lockfile, LockfilesData } from './types';

export function listLockfiles(workspaceId: string, projectId: string) {
  return api.post<LockfilesData>('/api/lockfiles/list', {
    workspace_id: workspaceId,
    project_id: projectId
  });
}

export function uploadLockfile(input: {
  workspaceId: string;
  projectId: string;
  uploadedByUserId: string;
  lockfileJson: string;
  checksumSha256: string;
  source?: string;
}) {
  return api.post<{ lockfile: Lockfile }>('/api/lockfiles/upload', {
    workspace_id: input.workspaceId,
    project_id: input.projectId,
    uploaded_by_user_id: input.uploadedByUserId,
    lockfile_json: input.lockfileJson,
    checksum_sha256: input.checksumSha256,
    source: input.source ?? 'dashboard'
  });
}


export function deleteLockfile(workspaceId: string, projectId: string, lockfileId: string) {
  return api.post<{ lockfile: Lockfile }>('/api/lockfiles/delete', { workspace_id: workspaceId, project_id: projectId, lockfile_id: lockfileId });
}

export function restoreLockfile(workspaceId: string, projectId: string, lockfileId: string) {
  return api.post<{ lockfile: Lockfile }>('/api/lockfiles/restore', { workspace_id: workspaceId, project_id: projectId, lockfile_id: lockfileId });
}
