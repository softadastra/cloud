<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { listMembers } from '$lib/api/members';
  import { createWorkspaceInvite, listWorkspaceInvites, revokeWorkspaceInvite } from '$lib/api/workspaceInvites';
  import { listWorkspaces } from '$lib/api/workspaces';
  import { ApiError, type Member, type Workspace, type WorkspaceInvite } from '$lib/api/types';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import RoleBadge from '$lib/components/RoleBadge.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import { canManageMembers, canRevokeInvite } from '$lib/permissions';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  let workspaces: Workspace[] = [];
  let members: Member[] = [];
  let invites: WorkspaceInvite[] = [];
  let selectedWorkspaceId = '';
  let email = '';
  let role = 'member';
  let loading = true;
  let saving = false;
  let error = '';
  let success = '';

  $: selectedWorkspace = workspaces.find((workspace) => workspace.id === selectedWorkspaceId) ?? null;
  $: currentRole = selectedWorkspace?.current_user_role ?? 'viewer';
  $: pendingInvites = invites.filter((invite) => invite.status === 'pending');
  $: inviteHistory = invites.filter((invite) => invite.status !== 'pending');

  async function load() {
    const user = $auth.user;
    if (!$auth.session || !user) {
      await goto('/login');
      return;
    }
    loading = true;
    error = '';
    try {
      const workspaceData = await listWorkspaces(user.id);
      workspaces = workspaceData.workspaces;
      const preferredId = new URLSearchParams(window.location.search).get('workspace_id') ?? '';
      selectedWorkspaceId = preferredId || workspaces[0]?.id || '';
      workspaceContext.setWorkspaces(workspaces, selectedWorkspaceId);
      await loadMembers();
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to load members.';
    } finally {
      loading = false;
    }
  }

  async function loadMembers() {
    if (!selectedWorkspaceId) {
      members = [];
      invites = [];
      return;
    }
    workspaceContext.setSelectedWorkspace(selectedWorkspaceId);
    const memberData = await listMembers(selectedWorkspaceId);
    members = memberData.members;

    if (canManageMembers(currentRole)) {
      const inviteData = await listWorkspaceInvites(selectedWorkspaceId);
      invites = inviteData.invites;
    } else {
      invites = [];
    }
  }

  async function submitInvite() {
    const user = $auth.user;
    if (!user || !selectedWorkspaceId || !email.trim() || !canManageMembers(currentRole)) return;
    saving = true;
    error = '';
    success = '';
    try {
      const invited = await createWorkspaceInvite({ workspaceId: selectedWorkspaceId, invitedEmail: email.trim(), role, invitedByUserId: user.id });
      invites = [invited.invite, ...invites];
      success = `Invitation sent to ${email.trim()}.`;
      email = '';
      role = 'member';
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to create invitation.';
    } finally {
      saving = false;
    }
  }

  async function revokeInvite(invite: WorkspaceInvite) {
    const user = $auth.user;
    if (!user || !canRevokeInvite(currentRole)) return;
    if (!confirm(`Revoke the invitation for ${invite.invited_email}?`)) return;
    error = '';
    success = '';
    try {
      const revoked = await revokeWorkspaceInvite({ workspaceId: invite.workspace_id, inviteId: invite.id, revokedByUserId: user.id });
      invites = invites.map((item) => (item.id === invite.id ? revoked.invite : item));
      success = 'Invitation revoked.';
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to revoke invitation.';
    }
  }

  onMount(load);
</script>

<svelte:head><title>Members | Softadastra Cloud</title></svelte:head>
<PageHeader eyebrow="Members" title="Workspace members" workspaceName={selectedWorkspace?.name ?? ''} role={currentRole} />
<InlineError message={error} />
{#if success}<p class="success-message">{success}</p>{/if}

<section class="dashboard-grid">
  <div class="panel">
    <div class="panel-header"><h2>Workspace</h2></div>
    <label>Workspace<select bind:value={selectedWorkspaceId} on:change={loadMembers}>{#each workspaces as workspace}<option value={workspace.id}>{workspace.name}</option>{/each}</select></label>
    {#if canManageMembers(currentRole)}
      <form on:submit|preventDefault={submitInvite}>
        <div class="panel-header"><h2>Invite member</h2></div>
        <label>Email<input bind:value={email} type="email" placeholder="teammate@example.com" required /></label>
        <label>Role<select bind:value={role}><option value="admin">admin</option><option value="member">member</option><option value="viewer">viewer</option></select></label>
        <button type="submit" disabled={saving || !selectedWorkspaceId}>{saving ? 'Sending...' : 'Send invitation'}</button>
      </form>
    {:else}
      <ReadOnlyNotice message="You can view members, but only owners and admins can invite or manage members." />
    {/if}
  </div>

  <div class="panel span-2">
    {#if canManageMembers(currentRole)}
      <div class="panel-header"><h2>Pending invitations</h2><span>{pendingInvites.length}</span></div>
      <div class="table-list compact">
        {#each pendingInvites as invite}
          <div class="row"><span><strong>{invite.invited_email}</strong><small><RoleBadge role={invite.role} /> <StatusBadge status={invite.status} /></small></span><button class="small danger" type="button" on:click={() => revokeInvite(invite)}>Revoke</button></div>
        {:else}<EmptyState title="No pending invitations." />{/each}
      </div>

      {#if inviteHistory.length > 0}
        <div class="panel-header"><h2>Invitation history</h2><span>{inviteHistory.length}</span></div>
        <div class="table-list compact">{#each inviteHistory as invite}<div class="row"><span><strong>{invite.invited_email}</strong><small><RoleBadge role={invite.role} /> <StatusBadge status={invite.status} /></small></span></div>{/each}</div>
      {/if}
    {/if}

    <div class="panel-header"><h2>Members</h2><span>{members.length}</span></div>
    {#if loading}<p class="muted">Loading...</p>{:else}
      <div class="table-list">
        {#each members as member}
          <div class="row"><span><strong>{member.email || 'Member'}</strong><small><RoleBadge role={member.role} /> <StatusBadge status={member.status} /></small></span><details><summary>Developer details</summary><small>{member.user_id}</small></details></div>
        {:else}<EmptyState title="No members yet." />{/each}
      </div>
    {/if}
  </div>
</section>
