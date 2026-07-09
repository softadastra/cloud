import { api } from './client';
import type { WorkspaceInvite, WorkspaceInvitesData } from './types';

export function createWorkspaceInvite(input: {
  workspaceId: string;
  invitedEmail: string;
  role: string;
  invitedByUserId: string;
  accessScope?: string;
  projectIdsJson?: string;
}) {
  return api.post<{ invite: WorkspaceInvite }>('/api/workspace_invites/create', {
    workspace_id: input.workspaceId,
    invited_email: input.invitedEmail,
    role: input.role,
    invited_by_user_id: input.invitedByUserId,
    access_scope: input.accessScope ?? 'entire_workspace',
    project_ids_json: input.projectIdsJson ?? '' 
  });
}

export function listWorkspaceInvites(workspaceId: string) {
  return api.post<WorkspaceInvitesData>('/api/workspace_invites/list', { workspace_id: workspaceId });
}

export function listMyWorkspaceInvites(userId: string, email: string) {
  return api.post<WorkspaceInvitesData>('/api/workspace_invites/list_mine', { user_id: userId, email });
}

export function acceptWorkspaceInvite(inviteId: string, userId: string, email: string) {
  return api.post<{ invite: WorkspaceInvite }>('/api/workspace_invites/accept', { invite_id: inviteId, user_id: userId, email });
}

export function declineWorkspaceInvite(inviteId: string, userId: string, email: string) {
  return api.post<{ invite: WorkspaceInvite }>('/api/workspace_invites/decline', { invite_id: inviteId, user_id: userId, email });
}

export function revokeWorkspaceInvite(input: { workspaceId: string; inviteId: string; revokedByUserId: string }) {
  return api.post<{ invite: WorkspaceInvite }>('/api/workspace_invites/revoke', {
    workspace_id: input.workspaceId,
    invite_id: input.inviteId,
    revoked_by_user_id: input.revokedByUserId
  });
}
