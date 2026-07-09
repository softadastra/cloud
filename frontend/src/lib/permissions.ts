export type WorkspaceRole = 'owner' | 'admin' | 'member' | 'viewer' | string | undefined | null;

function normalized(role: WorkspaceRole) {
  return (role || 'viewer').toLowerCase();
}

export function isOwner(role: WorkspaceRole) { return normalized(role) === 'owner'; }
export function isAdmin(role: WorkspaceRole) { return normalized(role) === 'admin'; }
export function isMember(role: WorkspaceRole) { return normalized(role) === 'member'; }
export function isViewer(role: WorkspaceRole) { return normalized(role) === 'viewer'; }
export function canRead(role: WorkspaceRole) { return ['owner', 'admin', 'member', 'viewer'].includes(normalized(role)); }
export function canManageMembers(role: WorkspaceRole) { return isOwner(role) || isAdmin(role); }
export function canManageTokens(role: WorkspaceRole) { return isOwner(role) || isAdmin(role); }
export function canWriteBusiness(role: WorkspaceRole) { return isOwner(role) || isAdmin(role) || isMember(role); }
export function canCreateProject(role: WorkspaceRole) { return canWriteBusiness(role); }
export function canCreatePackage(role: WorkspaceRole) { return canWriteBusiness(role); }
export function canPublishPackageVersion(role: WorkspaceRole) { return canWriteBusiness(role); }
export function canUploadLockfile(role: WorkspaceRole) { return canWriteBusiness(role); }
export function canSubmitBuildReport(role: WorkspaceRole) { return canWriteBusiness(role); }
export function canArchiveProject(role: WorkspaceRole) { return isOwner(role) || isAdmin(role); }
export function canArchivePackage(role: WorkspaceRole) { return isOwner(role) || isAdmin(role); }
export function canRevokeToken(role: WorkspaceRole) { return canManageTokens(role); }
export function canRevokeInvite(role: WorkspaceRole) { return canManageMembers(role); }
export function canRemoveMember(role: WorkspaceRole) { return canManageMembers(role); }
export function canChangeMemberRole(role: WorkspaceRole) { return canManageMembers(role); }

export function readableRole(role: WorkspaceRole) {
  const value = normalized(role);
  return ['owner', 'admin', 'member', 'viewer'].includes(value) ? value : 'viewer';
}
