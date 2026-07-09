<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { listMembers, reactivateMember, removeMember, suspendMember, updateMemberRole } from '$lib/api/members';
  import { listProjects } from '$lib/api/projects';
  import { createWorkspaceInvite, listWorkspaceInvites, revokeWorkspaceInvite } from '$lib/api/workspaceInvites';
  import { listWorkspaces } from '$lib/api/workspaces';
  import { ApiError, type Member, type Project, type Workspace, type WorkspaceInvite } from '$lib/api/types';
  import AccessScopeBadge from '$lib/components/AccessScopeBadge.svelte';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import LimitedAccessNotice from '$lib/components/LimitedAccessNotice.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import RoleBadge from '$lib/components/RoleBadge.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import { canManageMembersContext } from '$lib/permissions';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  let workspaces: Workspace[] = [];
  let projects: Project[] = [];
  let members: Member[] = [];
  let invites: WorkspaceInvite[] = [];
  let selectedWorkspaceId = '';
  let email = '';
  let role = 'member';
  let accessScope = 'entire_workspace';
  let selectedProjectIds: string[] = [];
  let loading = true;
  let saving = false;
  let busyId = '';
  let error = '';
  let success = '';

  $: selectedWorkspace = workspaces.find((workspace) => workspace.id === selectedWorkspaceId) ?? null;
  $: permissionContext = {
    role: selectedWorkspace?.current_user_role ?? 'viewer',
    status: selectedWorkspace?.current_user_status ?? 'active',
    access_scope: selectedWorkspace?.access_scope ?? 'entire_workspace',
    project_ids_json: selectedWorkspace?.project_ids_json ?? ''
  };
  $: currentRole = permissionContext.role;
  $: canManage = canManageMembersContext(permissionContext);
  $: pendingInvites = invites.filter((invite) => invite.status === 'pending');
  $: inviteHistory = invites.filter((invite) => invite.status !== 'pending');
  $: activeMembers = members.filter((member) => member.status === 'active');
  $: suspendedMembers = members.filter((member) => member.status === 'suspended');
  $: removedMembers = members.filter((member) => member.status === 'removed');

  function parseProjectIds(value?: string) {
    if (!value) return [];
    try { return JSON.parse(value) as string[]; } catch { return []; }
  }

  function projectNames(value?: string) {
    const ids = parseProjectIds(value);
    if (ids.length === 0) return 'All projects';
    return ids.map((id) => projects.find((project) => project.id === id)?.name ?? id).join(', ');
  }

  function toggleProject(projectId: string) {
    selectedProjectIds = selectedProjectIds.includes(projectId)
      ? selectedProjectIds.filter((id) => id !== projectId)
      : [...selectedProjectIds, projectId];
  }

  async function load() {
    const user = $auth.user;
    if (!$auth.session || !user) { await goto('/login'); return; }
    loading = true; error = '';
    try {
      const workspaceData = await listWorkspaces(user.id);
      workspaces = workspaceData.workspaces;
      const preferredId = new URLSearchParams(window.location.search).get('workspace_id') ?? '';
      selectedWorkspaceId = preferredId || workspaces[0]?.id || '';
      workspaceContext.setWorkspaces(workspaces, selectedWorkspaceId);
      await loadMembers();
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to load members.';
    } finally { loading = false; }
  }

  async function loadMembers() {
    if (!selectedWorkspaceId) { members = []; invites = []; projects = []; return; }
    workspaceContext.setSelectedWorkspace(selectedWorkspaceId);
    const [memberData, projectData] = await Promise.all([listMembers(selectedWorkspaceId), listProjects(selectedWorkspaceId)]);
    members = memberData.members;
    projects = projectData.projects;
    selectedProjectIds = [];
    if (canManage) {
      const inviteData = await listWorkspaceInvites(selectedWorkspaceId);
      invites = inviteData.invites;
    } else {
      invites = [];
    }
  }

  async function submitInvite() {
    const user = $auth.user;
    if (!user || !selectedWorkspaceId || !email.trim() || !canManage) return;
    if (accessScope === 'selected_projects' && selectedProjectIds.length === 0) { error = 'Choose at least one project for selected project access.'; return; }
    saving = true; error = ''; success = '';
    try {
      const invited = await createWorkspaceInvite({
        workspaceId: selectedWorkspaceId,
        invitedEmail: email.trim(),
        role,
        invitedByUserId: user.id,
        accessScope,
        projectIdsJson: accessScope === 'selected_projects' ? JSON.stringify(selectedProjectIds) : ''
      });
      invites = [invited.invite, ...invites];
      success = `Invitation sent to ${email.trim()}.`;
      email = ''; role = 'member'; accessScope = 'entire_workspace'; selectedProjectIds = [];
    } catch (err) { error = err instanceof ApiError ? err.message : 'Unable to create invitation.'; }
    finally { saving = false; }
  }

  async function revokeInvite(invite: WorkspaceInvite) {
    const user = $auth.user; if (!user || !canManage) return;
    if (!confirm(`Revoke the invitation for ${invite.invited_email}?`)) return;
    busyId = invite.id; error = ''; success = '';
    try { const revoked = await revokeWorkspaceInvite({ workspaceId: invite.workspace_id, inviteId: invite.id, revokedByUserId: user.id }); invites = invites.map((item) => item.id === invite.id ? revoked.invite : item); success = 'Invitation revoked.'; }
    catch (err) { error = err instanceof ApiError ? err.message : 'Unable to revoke invitation.'; }
    finally { busyId = ''; }
  }

  async function changeRole(member: Member, nextRole: string) {
    const user = $auth.user; if (!user || !canManage || nextRole === member.role) return;
    busyId = member.id; error = ''; success = '';
    try { const updated = await updateMemberRole({ workspaceId: member.workspace_id, userId: member.user_id, role: nextRole, actorUserId: user.id }); members = members.map((item) => item.id === member.id ? updated.member : item); success = 'Member role updated.'; }
    catch (err) { error = err instanceof ApiError ? err.message : 'Unable to update role.'; }
    finally { busyId = ''; }
  }

  async function lifecycle(member: Member, action: 'suspend' | 'reactivate' | 'remove') {
    const user = $auth.user; if (!user || !canManage) return;
    const label = action === 'reactivate' ? 'reactivate' : action;
    if (!confirm(`${label} ${member.email}?`)) return;
    busyId = member.id; error = ''; success = '';
    try {
      const fn = action === 'suspend' ? suspendMember : action === 'reactivate' ? reactivateMember : removeMember;
      const updated = await fn({ workspaceId: member.workspace_id, userId: member.user_id, actorUserId: user.id });
      members = members.map((item) => item.id === member.id ? updated.member : item);
      success = `Member ${label}d.`;
    } catch (err) { error = err instanceof ApiError ? err.message : `Unable to ${label} member.`; }
    finally { busyId = ''; }
  }

  onMount(load);
</script>

<svelte:head><title>Members | Softadastra Cloud</title></svelte:head>
<PageHeader eyebrow="Members" title="Workspace access" workspaceName={selectedWorkspace?.name ?? ''} role={currentRole} />
<InlineError message={error} />
{#if success}<p class="success-message">{success}</p>{/if}
{#if permissionContext.access_scope === 'selected_projects'}<LimitedAccessNotice />{/if}

<section class="dashboard-grid">
  <div class="panel">
    <div class="panel-header"><h2>Workspace</h2></div>
    <label>Workspace<select bind:value={selectedWorkspaceId} on:change={loadMembers}>{#each workspaces as workspace}<option value={workspace.id}>{workspace.name}</option>{/each}</select></label>
    {#if canManage}
      <form on:submit|preventDefault={submitInvite}>
        <div class="panel-header"><h2>Invite member</h2></div>
        <label>Email<input bind:value={email} type="email" placeholder="teammate@example.com" required /></label>
        <label>Role<select bind:value={role}><option value="admin">admin</option><option value="member">member</option><option value="viewer">viewer</option></select></label>
        <label>Access scope<select bind:value={accessScope}><option value="entire_workspace">Entire workspace</option><option value="selected_projects">Selected projects</option></select></label>
        {#if accessScope === 'selected_projects'}
          <p class="muted">Selected projects means this user will only see the projects you choose.</p>
          <div class="checkbox-list">{#each projects as project}<label><input type="checkbox" checked={selectedProjectIds.includes(project.id)} on:change={() => toggleProject(project.id)} /> {project.name}</label>{:else}<p class="muted">Create a project before using selected project access.</p>{/each}</div>
        {/if}
        <button type="submit" disabled={saving || !selectedWorkspaceId || (accessScope === 'selected_projects' && selectedProjectIds.length === 0)}>{saving ? 'Sending...' : 'Send invitation'}</button>
      </form>
    {:else}
      <ReadOnlyNotice message="You can view members, but only owners and admins can manage access." />
    {/if}
  </div>

  <div class="panel span-2">
    {#if canManage}
      <div class="panel-header"><h2>Pending invitations</h2><span>{pendingInvites.length}</span></div>
      <div class="table-list compact">
        {#each pendingInvites as invite}
          <div class="row"><span><strong>{invite.invited_email}</strong><small><RoleBadge role={invite.role} /> <AccessScopeBadge scope={invite.access_scope} /> <StatusBadge status={invite.status} /> · {projectNames(invite.project_ids_json)}</small></span><button class="small danger" type="button" disabled={busyId === invite.id} on:click={() => revokeInvite(invite)}>Revoke</button></div>
        {:else}<EmptyState title="No pending invitations." />{/each}
      </div>
      {#if inviteHistory.length > 0}<div class="panel-header"><h2>Invitation history</h2><span>{inviteHistory.length}</span></div><div class="table-list compact">{#each inviteHistory as invite}<div class="row"><span><strong>{invite.invited_email}</strong><small><RoleBadge role={invite.role} /> <AccessScopeBadge scope={invite.access_scope} /> <StatusBadge status={invite.status} /></small></span></div>{/each}</div>{/if}
    {/if}

    <div class="panel-header"><h2>Active members</h2><span>{activeMembers.length}</span></div>
    <div class="table-list">
      {#each activeMembers as member}
        <div class="row"><span><strong>{member.email || 'Member'}</strong><small><RoleBadge role={member.role} /> <AccessScopeBadge scope={member.access_scope} /> <StatusBadge status={member.status} /> · {projectNames(member.project_ids_json)}</small></span>{#if canManage}<span class="actions"><select value={member.role} disabled={busyId === member.id} on:change={(event) => changeRole(member, event.currentTarget.value)}><option value="admin">admin</option><option value="member">member</option><option value="viewer">viewer</option></select><button class="small" disabled={busyId === member.id} on:click={() => lifecycle(member, 'suspend')}>Suspend</button><button class="small danger" disabled={busyId === member.id} on:click={() => lifecycle(member, 'remove')}>Remove</button></span>{/if}</div>
      {:else}<EmptyState title="No active members." />{/each}
    </div>

    {#if suspendedMembers.length > 0}<div class="panel-header"><h2>Suspended members</h2><span>{suspendedMembers.length}</span></div><div class="table-list">{#each suspendedMembers as member}<div class="row muted-row"><span><strong>{member.email}</strong><small><RoleBadge role={member.role} /> <AccessScopeBadge scope={member.access_scope} /> <StatusBadge status={member.status} /></small></span>{#if canManage}<button class="small" disabled={busyId === member.id} on:click={() => lifecycle(member, 'reactivate')}>Reactivate</button>{/if}</div>{/each}</div>{/if}
    {#if removedMembers.length > 0}<div class="panel-header"><h2>Removed members</h2><span>{removedMembers.length}</span></div><div class="table-list">{#each removedMembers as member}<div class="row muted-row"><span><strong>{member.email}</strong><small><RoleBadge role={member.role} /> <AccessScopeBadge scope={member.access_scope} /> <StatusBadge status={member.status} /></small></span>{#if canManage}<button class="small" disabled={busyId === member.id} on:click={() => lifecycle(member, 'reactivate')}>Reactivate</button>{/if}</div>{/each}</div>{/if}
  </div>
</section>
