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
