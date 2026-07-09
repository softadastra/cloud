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


export type PermissionContext = {
  role?: WorkspaceRole;
  status?: 'active' | 'suspended' | 'removed' | string | null;
  access_scope?: 'entire_workspace' | 'selected_projects' | string | null;
  project_ids?: string[];
  project_ids_json?: string;
  projectId?: string | null;
};

function active(ctx: PermissionContext) { return (ctx.status || 'active') === 'active'; }
function ctxRole(ctx: PermissionContext) { return ctx.role || 'viewer'; }
export function hasWorkspaceWideAccess(ctx: PermissionContext) { return (ctx.access_scope || 'entire_workspace') === 'entire_workspace'; }
export function hasSelectedProjectAccess(ctx: PermissionContext) { return (ctx.access_scope || 'entire_workspace') === 'selected_projects'; }
export function projectIdsFromContext(ctx: PermissionContext) {
  if (ctx.project_ids) return ctx.project_ids;
  if (!ctx.project_ids_json) return [];
  try { return JSON.parse(ctx.project_ids_json) as string[]; } catch { return []; }
}
export function canAccessProject(ctx: PermissionContext, projectId: string) {
  if (!active(ctx)) return false;
  if (hasWorkspaceWideAccess(ctx)) return true;
  return projectIdsFromContext(ctx).includes(projectId);
}
export function canManageMembersContext(ctx: PermissionContext) { return active(ctx) && hasWorkspaceWideAccess(ctx) && canManageMembers(ctxRole(ctx)); }
export function canManageTokensContext(ctx: PermissionContext) { return active(ctx) && hasWorkspaceWideAccess(ctx) && canManageTokens(ctxRole(ctx)); }
export function canReadProjects(ctx: PermissionContext) { return active(ctx) && canRead(ctxRole(ctx)); }
export function canCreateProjectContext(ctx: PermissionContext) { return active(ctx) && hasWorkspaceWideAccess(ctx) && canCreateProject(ctxRole(ctx)); }
export function canUpdateProject(ctx: PermissionContext) { return active(ctx) && hasWorkspaceWideAccess(ctx) && canWriteBusiness(ctxRole(ctx)); }
export function canCreatePackageContext(ctx: PermissionContext) { return active(ctx) && canCreatePackage(ctxRole(ctx)); }
export function canPublishPackageVersionContext(ctx: PermissionContext) { return active(ctx) && canPublishPackageVersion(ctxRole(ctx)); }
export function canUploadLockfileContext(ctx: PermissionContext) { return active(ctx) && canUploadLockfile(ctxRole(ctx)); }
export function canSubmitBuildReportContext(ctx: PermissionContext) { return active(ctx) && canSubmitBuildReport(ctxRole(ctx)); }
export function canReadOnly(ctx: PermissionContext) { return active(ctx) && isViewer(ctxRole(ctx)); }
