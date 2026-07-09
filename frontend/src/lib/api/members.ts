import { api } from './client';
import type { Member, MembersData } from './types';

export function listMembers(workspaceId: string) {
  return api.post<MembersData>('/api/members/list', { workspace_id: workspaceId });
}

export function inviteMember(input: {
  workspaceId: string;
  userId?: string;
  email: string;
  role: string;
  invitedByUserId: string;
}) {
  return api.post<{ member: Member }>('/api/members/invite', {
    workspace_id: input.workspaceId,
    user_id: input.userId ?? '',
    email: input.email,
    role: input.role,
    invited_by_user_id: input.invitedByUserId
  });
}


export function updateMemberRole(input: { workspaceId: string; userId: string; role: string; actorUserId: string }) {
  return api.post<{ member: Member }>('/api/members/update_role', {
    workspace_id: input.workspaceId,
    user_id: input.userId,
    role: input.role,
    actor_user_id: input.actorUserId
  });
}

export function suspendMember(input: { workspaceId: string; userId: string; actorUserId: string }) {
  return api.post<{ member: Member }>('/api/members/suspend', {
    workspace_id: input.workspaceId,
    user_id: input.userId,
    actor_user_id: input.actorUserId
  });
}

export function reactivateMember(input: { workspaceId: string; userId: string; actorUserId: string }) {
  return api.post<{ member: Member }>('/api/members/reactivate', {
    workspace_id: input.workspaceId,
    user_id: input.userId,
    actor_user_id: input.actorUserId
  });
}

export function removeMember(input: { workspaceId: string; userId: string; actorUserId: string }) {
  return api.post<{ member: Member }>('/api/members/remove', {
    workspace_id: input.workspaceId,
    user_id: input.userId,
    actor_user_id: input.actorUserId
  });
}
