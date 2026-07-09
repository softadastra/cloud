import { api } from './client';
import type { CliToken, TokensData } from './types';

export const defaultCliScopes = [
  'registry:read',
  'registry:write',
  'projects:read',
  'projects:write',
  'lockfiles:write',
  'build_reports:write'
];

export function listTokens(workspaceId: string, userId: string) {
  return api.post<TokensData>('/api/tokens/list', { workspace_id: workspaceId, user_id: userId });
}

export function createToken(input: { workspaceId: string; userId: string; name: string; scopes?: string[] }) {
  return api.post<{ token: CliToken }>('/api/tokens/create', {
    workspace_id: input.workspaceId,
    user_id: input.userId,
    name: input.name,
    scopes: input.scopes ?? defaultCliScopes
  });
}

export function revokeToken(input: { workspaceId: string; userId: string; tokenId: string }) {
  return api.post<{ token: CliToken }>('/api/tokens/revoke', {
    workspace_id: input.workspaceId,
    user_id: input.userId,
    token_id: input.tokenId
  });
}
