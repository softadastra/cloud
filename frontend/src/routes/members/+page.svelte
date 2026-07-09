<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { listMembers } from '$lib/api/members';
  import { createWorkspaceInvite, listWorkspaceInvites, revokeWorkspaceInvite } from '$lib/api/workspaceInvites';
  import { listWorkspaces } from '$lib/api/workspaces';
  import { ApiError, type Member, type Workspace, type WorkspaceInvite } from '$lib/api/types';
  import { auth } from '$lib/stores/auth';

  let workspaces: Workspace[] = [];
  let members: Member[] = [];
  let invites: WorkspaceInvite[] = [];
  let selectedWorkspaceId = '';
  let email = '';
  let role = 'member';
  let loading = true;
  let saving = false;
  let error = '';

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
      selectedWorkspaceId = new URLSearchParams(window.location.search).get('workspace_id') ?? workspaces[0]?.id ?? '';
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
    const [memberData, inviteData] = await Promise.all([listMembers(selectedWorkspaceId), listWorkspaceInvites(selectedWorkspaceId)]);
    members = memberData.members;
    invites = inviteData.invites;
  }

  async function submitInvite() {
    const user = $auth.user;
    if (!user || !selectedWorkspaceId || !email.trim()) return;
    saving = true;
    error = '';
    try {
      const invited = await createWorkspaceInvite({ workspaceId: selectedWorkspaceId, invitedEmail: email.trim(), role, invitedByUserId: user.id });
      invites = [invited.invite, ...invites];
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
    if (!user) return;
    error = '';
    try {
      const revoked = await revokeWorkspaceInvite({ workspaceId: invite.workspace_id, inviteId: invite.id, revokedByUserId: user.id });
      invites = invites.map((item) => (item.id === invite.id ? revoked.invite : item));
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to revoke invitation.';
    }
  }

  onMount(load);
</script>

<svelte:head><title>Members | Softadastra Cloud</title></svelte:head>
<section class="page-header"><div><p class="eyebrow">Members</p><h1>Workspace members</h1></div></section>
{#if error}<p class="form-error">{error}</p>{/if}

<section class="dashboard-grid">
  <form class="panel" on:submit|preventDefault={submitInvite}>
    <div class="panel-header"><h2>Invite member</h2></div>
    <label>Workspace<select bind:value={selectedWorkspaceId} on:change={loadMembers}>{#each workspaces as workspace}<option value={workspace.id}>{workspace.name}</option>{/each}</select></label>
    <label>Email<input bind:value={email} type="email" placeholder="teammate@example.com" required /></label>
    <label>Role<select bind:value={role}><option value="admin">admin</option><option value="member">member</option><option value="viewer">viewer</option></select></label>
    <button type="submit" disabled={saving || !selectedWorkspaceId}>{saving ? 'Inviting...' : 'Send invitation'}</button>
  </form>

  <div class="panel span-2">
    <div class="panel-header"><h2>Pending invitations</h2><span>{invites.filter((invite) => invite.status === 'pending').length}</span></div>
    <div class="table-list compact">
      {#each invites as invite}
        <div class="row"><span><strong>{invite.invited_email}</strong><small>{invite.role} / {invite.status}</small></span>{#if invite.status === 'pending'}<button class="small danger" type="button" on:click={() => revokeInvite(invite)}>Revoke</button>{/if}</div>
      {:else}<p class="muted padded">No invitations.</p>{/each}
    </div>

    <div class="panel-header"><h2>Members</h2><span>{members.length}</span></div>
    {#if loading}<p class="muted">Loading...</p>{:else}
      <div class="table-list">
        {#each members as member}
          <div class="row"><span><strong>{member.email}</strong><small>{member.user_id}</small></span><span class="actions"><small>{member.role}</small><small>{member.status}</small></span></div>
        {:else}<p class="muted padded">No members.</p>{/each}
      </div>
    {/if}
  </div>
</section>
